// Floating Temple
// Copyright 2015 Derek S. Snyder
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PROTOCOL_SERVER_PROTOCOL_SERVER_H_
#define PROTOCOL_SERVER_PROTOCOL_SERVER_H_

#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <unordered_set>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "base/mutex_lock.h"
#include "protocol_server/protocol_connection.h"
#include "protocol_server/protocol_connection_handler.h"
#include "protocol_server/protocol_connection_impl.h"
#include "protocol_server/protocol_server_handler.h"
#include "protocol_server/protocol_server_interface_for_connection.h"
#include "util/event_fd.h"
#include "util/producer_consumer_queue.h"
#include "util/socket_util.h"
#include "util/state_variable.h"
#include "util/tcp.h"

DECLARE_int32(protocol_connection_timeout_sec_for_debugging);

namespace floating_temple {

template<class Message>
class ProtocolServer : private ProtocolServerInterfaceForConnection {
 public:
  ProtocolServer();
  ~ProtocolServer();

  void Start(ProtocolServerHandler<Message>* handler,
             const std::string& local_address,
             int listen_port,
             int send_receive_thread_count);
  void Stop();

  // This method does not take ownership of *connection_handler. The caller must
  // take ownership of the returned ProtocolConnection instance.
  ProtocolConnection* OpenConnection(
      ProtocolConnectionHandler<Message>* connection_handler,
      const std::string& address, int port);

 private:
  enum {
    NOT_STARTED = 0x1,
    STARTING = 0x2,
    RUNNING = 0x4,
    STOPPING = 0x8,
    STOPPED = 0x10
  };

  bool AcceptSingleConnection();

  ProtocolConnectionImpl<Message>* CreateConnection(
      ProtocolConnectionHandler<Message>* connection_handler, int socket_fd,
      const std::string& remote_address);

  void DoSelectLoop();
  void SendAndReceiveData();

  bool GetNextReadyConnection(ProtocolConnectionImpl<Message>** connection);
  void AddConnectionToReadyConnections(
      ProtocolConnectionImpl<Message>* connection);

  void NotifyConnectionsChanged() override;

  static void* SelectThreadMain(void* protocol_server_raw);
  static void* SendReceiveThreadMain(void* protocol_server_raw);

  static void AddFdToSet(int fd, fd_set* fds, int* nfds);

  ProtocolServerHandler<Message>* handler_;
  int listen_fd_;
  // Signaled when a connection is added to blocked_connections_, or when the
  // state of a connection changes.
  int connections_changed_event_fd_;
  pthread_t select_thread_;
  std::vector<pthread_t> send_receive_threads_;

  // A NULL pointer in this queue means that the listen socket is ready. (I.e.,
  // an incoming connection is ready to be accepted.)
  ProducerConsumerQueue<ProtocolConnectionImpl<Message>*> ready_connections_;

  // A NULL pointer in this set means that the listen socket is blocked.
  std::unordered_set<ProtocolConnectionImpl<Message>*> blocked_connections_;
  mutable Mutex blocked_connections_mu_;

  StateVariable state_;

  DISALLOW_COPY_AND_ASSIGN(ProtocolServer);
};

template<class Message>
ProtocolServer<Message>::ProtocolServer()
    : handler_(nullptr),
      listen_fd_(-1),
      connections_changed_event_fd_(-1),
      ready_connections_(-1),
      state_(NOT_STARTED) {
  state_.AddStateTransition(NOT_STARTED, STARTING);
  state_.AddStateTransition(STARTING, RUNNING);
  state_.AddStateTransition(RUNNING, STOPPING);
  state_.AddStateTransition(STOPPING, STOPPED);
}

template<class Message>
ProtocolServer<Message>::~ProtocolServer() {
  state_.CheckState(NOT_STARTED | STOPPED);

  MutexLock lock(&blocked_connections_mu_);
  CHECK_EQ(blocked_connections_.size(), 0u);
}

template<class Message>
void ProtocolServer<Message>::Start(ProtocolServerHandler<Message>* handler,
                                    const std::string& local_address,
                                    int listen_port,
                                    int send_receive_thread_count) {
  CHECK(handler != nullptr);
  CHECK_GT(send_receive_thread_count, 0);

  state_.ChangeState(STARTING);

  handler_ = handler;
  listen_fd_ = ListenOnLocalAddress(local_address, listen_port);

  connections_changed_event_fd_ = eventfd(0, EFD_CLOEXEC);
  CHECK_ERR(connections_changed_event_fd_) << " eventfd";
  CHECK(SetFdToNonBlocking(connections_changed_event_fd_));

  CHECK_PTHREAD_ERR(pthread_create(
      &select_thread_, nullptr,
      &ProtocolServer<Message>::SelectThreadMain, this));

  send_receive_threads_.resize(send_receive_thread_count);
  for (int i = 0; i < send_receive_thread_count; ++i) {
    CHECK_PTHREAD_ERR(pthread_create(
        &send_receive_threads_[i], nullptr,
        &ProtocolServer<Message>::SendReceiveThreadMain, this));
  }

  CHECK(ready_connections_.Push(nullptr, false));

  state_.ChangeState(RUNNING);
}

template<class Message>
void ProtocolServer<Message>::Stop() {
  state_.ChangeState(STOPPING);
  // Wake the send/receive thread.
  ready_connections_.Drain();
  // Wake the select thread.
  SignalEventFd(connections_changed_event_fd_);

  for (const pthread_t thread : send_receive_threads_) {
    void* thread_return_value = nullptr;
    CHECK_PTHREAD_ERR(pthread_join(thread, &thread_return_value));
  }

  void* thread_return_value = nullptr;
  CHECK_PTHREAD_ERR(pthread_join(select_thread_, &thread_return_value));

  CHECK_ERR(close(connections_changed_event_fd_));
  CHECK_ERR(close(listen_fd_));

  std::unordered_set<ProtocolConnectionImpl<Message>*> all_connections;

  for (;;) {
    ProtocolConnectionImpl<Message>* connection = nullptr;
    if (!ready_connections_.Pop(&connection, false)) {
      break;
    }

    if (connection != nullptr) {
      CHECK(all_connections.insert(connection).second);
    }
  }

  {
    MutexLock lock(&blocked_connections_mu_);

    for (ProtocolConnectionImpl<Message>* const connection :
             blocked_connections_) {
      if (connection != nullptr) {
        CHECK(all_connections.insert(connection).second);
      }
    }

    blocked_connections_.clear();
  }

  for (ProtocolConnectionImpl<Message>* const connection : all_connections) {
    connection->CloseSocket();
  }

  state_.ChangeState(STOPPED);
}

template<class Message>
ProtocolConnection* ProtocolServer<Message>::OpenConnection(
    ProtocolConnectionHandler<Message>* connection_handler,
    const std::string& address, int port) {
  const int socket_fd = ConnectToRemoteHost(address, port);
  if (socket_fd == -1) {
    return nullptr;
  }
  return CreateConnection(connection_handler, socket_fd, "");
}

template<class Message>
bool ProtocolServer<Message>::AcceptSingleConnection() {
  std::string remote_address;
  const int connection_fd = AcceptConnection(listen_fd_, &remote_address);

  if (connection_fd == -1) {
    return false;
  }

  CreateConnection(nullptr, connection_fd, remote_address);

  return true;
}

template<class Message>
ProtocolConnectionImpl<Message>* ProtocolServer<Message>::CreateConnection(
    ProtocolConnectionHandler<Message>* connection_handler, int socket_fd,
    const std::string& remote_address) {
  // TODO(dss): Should we set socket_fd to non-blocking mode here? (It's already
  // set to non-blocking mode elsewhere, but perhaps this is the proper place to
  // do it.)

  ProtocolConnectionImpl<Message>* const connection =
      new ProtocolConnectionImpl<Message>(this, socket_fd);

  if (connection_handler == nullptr) {
    connection_handler = handler_->NotifyConnectionReceived(connection,
                                                            remote_address);
  }

  connection->Init(connection_handler);
  AddConnectionToReadyConnections(connection);

  return connection;
}

template<class Message>
void ProtocolServer<Message>::DoSelectLoop() {
  state_.WaitForNotState(NOT_STARTED | STARTING);

  while (state_.MatchesStateMask(RUNNING)) {
    ClearEventFd(connections_changed_event_fd_);

    std::unordered_set<ProtocolConnectionImpl<Message>*>
        blocked_connections_temp;
    {
      MutexLock lock(&blocked_connections_mu_);
      blocked_connections_temp.swap(blocked_connections_);
    }

    int nfds = 0;

    fd_set read_fd_set, write_fd_set;
    FD_ZERO(&read_fd_set);
    FD_ZERO(&write_fd_set);

    for (typename std::unordered_set<ProtocolConnectionImpl<Message>*>::iterator
             connection_it = blocked_connections_temp.begin();
         connection_it != blocked_connections_temp.end(); ) {
      ProtocolConnectionImpl<Message>* const connection = *connection_it;

      bool erase_connection = false;

      if (connection == nullptr) {
        AddFdToSet(listen_fd_, &read_fd_set, &nfds);
      } else {
        if (connection->close_requested()) {
          connection->CloseSocket();
          handler_->NotifyConnectionClosed(
              connection->protocol_connection_handler());

          erase_connection = true;
        } else {
          if (connection->IsBlocked()) {
            const int socket_fd = connection->socket_fd();

            AddFdToSet(socket_fd, &read_fd_set, &nfds);
            if (connection->HasOutputData()) {
              AddFdToSet(socket_fd, &write_fd_set, &nfds);
            }
          } else {
            AddConnectionToReadyConnections(connection);
            erase_connection = true;
          }
        }
      }

      if (erase_connection) {
        const typename
            std::unordered_set<ProtocolConnectionImpl<Message>*>::iterator
            erase_it = connection_it;
        ++connection_it;
        blocked_connections_temp.erase(erase_it);
      } else {
        ++connection_it;
      }
    }

    AddFdToSet(connections_changed_event_fd_, &read_fd_set, &nfds);

    timeval deadline;
    deadline.tv_sec = 0;
    deadline.tv_usec = 0;

    timeval* deadline_ptr = nullptr;

    if (FLAGS_protocol_connection_timeout_sec_for_debugging >= 0) {
      deadline.tv_sec = static_cast<long>(
          FLAGS_protocol_connection_timeout_sec_for_debugging);
      deadline_ptr = &deadline;
    }

    // TODO(dss): Use epoll instead of select for better performance.
    VLOG(1) << "Entering select()";
    const int fd_count = select(nfds, &read_fd_set, &write_fd_set, nullptr,
                                deadline_ptr);
    VLOG(1) << "Exiting select()";
    CHECK_ERR(fd_count) << " select";
    CHECK_GT(fd_count, 0) << "select() timed out.";

    for (ProtocolConnectionImpl<Message>* const connection :
             blocked_connections_temp) {
      bool ready = false;

      if (connection == nullptr) {
        if (FD_ISSET(listen_fd_, &read_fd_set)) {
          ready = true;
        }
      } else {
        const int socket_fd = connection->socket_fd();

        if (FD_ISSET(socket_fd, &read_fd_set) ||
            FD_ISSET(socket_fd, &write_fd_set)) {
          ready = true;
        }
      }

      if (ready) {
        AddConnectionToReadyConnections(connection);
      } else {
        MutexLock lock(&blocked_connections_mu_);
        CHECK(blocked_connections_.insert(connection).second);
      }
    }
  }
}

template<class Message>
void ProtocolServer<Message>::SendAndReceiveData() {
  state_.WaitForNotState(NOT_STARTED | STARTING);

  for (;;) {
    ProtocolConnectionImpl<Message>* connection = nullptr;
    if (!GetNextReadyConnection(&connection)) {
      return;
    }

    bool blocked = false;

    if (connection == nullptr) {
      if (!AcceptSingleConnection()) {
        blocked = true;
      }
    } else {
      connection->SendAndReceive();

      if (connection->close_requested() || connection->IsBlocked()) {
        blocked = true;
      }
    }

    if (blocked) {
      {
        MutexLock lock(&blocked_connections_mu_);
        CHECK(blocked_connections_.insert(connection).second);
      }

      SignalEventFd(connections_changed_event_fd_);
    } else {
      AddConnectionToReadyConnections(connection);
    }
  }
}

template<class Message>
bool ProtocolServer<Message>::GetNextReadyConnection(
    ProtocolConnectionImpl<Message>** connection) {
  while (state_.MatchesStateMask(RUNNING)) {
    if (ready_connections_.Pop(connection, true)) {
      return true;
    }
  }

  return false;
}

template<class Message>
void ProtocolServer<Message>::AddConnectionToReadyConnections(
    ProtocolConnectionImpl<Message>* connection) {
  CHECK(ready_connections_.Push(connection, false));
}

template<class Message>
void ProtocolServer<Message>::NotifyConnectionsChanged() {
  SignalEventFd(connections_changed_event_fd_);
}

// static
template<class Message>
void* ProtocolServer<Message>::SelectThreadMain(void* protocol_server_raw) {
  CHECK(protocol_server_raw != nullptr);
  static_cast<ProtocolServer<Message>*>(protocol_server_raw)->DoSelectLoop();
  return nullptr;
}

// static
template<class Message>
void* ProtocolServer<Message>::SendReceiveThreadMain(
    void* protocol_server_raw) {
  CHECK(protocol_server_raw != nullptr);
  static_cast<ProtocolServer<Message>*>(protocol_server_raw)->
      SendAndReceiveData();
  return nullptr;
}

// static
template<class Message>
void ProtocolServer<Message>::AddFdToSet(int fd, fd_set* fds, int* nfds) {
  CHECK_GE(fd, 0);
  CHECK(fds != nullptr);
  CHECK(nfds != nullptr);
  CHECK_GE(*nfds, 0);

  VLOG(1) << "Adding FD " << fd << " to fd_set " << fds;

  FD_SET(fd, fds);

  if (fd + 1 > *nfds) {
    *nfds = fd + 1;
  }
}

}  // namespace floating_temple

#endif  // PROTOCOL_SERVER_PROTOCOL_SERVER_H_
