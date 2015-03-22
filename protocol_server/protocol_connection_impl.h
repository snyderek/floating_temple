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

#ifndef PROTOCOL_SERVER_PROTOCOL_CONNECTION_IMPL_H_
#define PROTOCOL_SERVER_PROTOCOL_CONNECTION_IMPL_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "protocol_server/format_protocol_message.h"
#include "protocol_server/parse_protocol_message.h"
#include "protocol_server/protocol_connection.h"
#include "protocol_server/protocol_connection_handler.h"
#include "protocol_server/protocol_server_interface_for_connection.h"

namespace floating_temple {

// This class is not thread-safe.
template<class Message>
class ProtocolConnectionImpl : public ProtocolConnection {
 public:
  ProtocolConnectionImpl(ProtocolServerInterfaceForConnection* protocol_server,
                         int socket_fd);
  virtual ~ProtocolConnectionImpl();

  void Init(ProtocolConnectionHandler<Message>* protocol_connection_handler);

  int socket_fd() const { return socket_fd_; }
  ProtocolConnectionHandler<Message>* protocol_connection_handler() const
      { return protocol_connection_handler_; }
  bool close_requested() const { return close_requested_; }

  bool IsBlocked();
  bool HasOutputData() { return PrivateHasOutputData(); }

  // Sends and receives data on the socket connection.
  void SendAndReceive();

  void CloseSocket();

  virtual void Close();
  virtual void NotifyMessageReadyToSend();

 private:
  void ParseMessages(std::string* input_string);
  bool PrivateHasOutputData();

  ProtocolServerInterfaceForConnection* const protocol_server_;
  const int socket_fd_;

  ProtocolConnectionHandler<Message>* protocol_connection_handler_;

  bool receive_blocked_;
  bool send_blocked_;
  bool close_requested_;

  std::string input_data_;
  std::string output_data_;

  DISALLOW_COPY_AND_ASSIGN(ProtocolConnectionImpl);
};

template<class Message>
ProtocolConnectionImpl<Message>::ProtocolConnectionImpl(
    ProtocolServerInterfaceForConnection* protocol_server, int socket_fd)
    : protocol_server_(CHECK_NOTNULL(protocol_server)),
      socket_fd_(socket_fd),
      protocol_connection_handler_(nullptr),
      receive_blocked_(false),
      send_blocked_(false),
      close_requested_(false) {
  CHECK_NE(socket_fd, -1);
}

template<class Message>
ProtocolConnectionImpl<Message>::~ProtocolConnectionImpl() {
}

template<class Message>
void ProtocolConnectionImpl<Message>::Init(
    ProtocolConnectionHandler<Message>* protocol_connection_handler) {
  CHECK(protocol_connection_handler_ == nullptr);
  CHECK(protocol_connection_handler != nullptr);

  protocol_connection_handler_ = protocol_connection_handler;
}

template<class Message>
bool ProtocolConnectionImpl<Message>::IsBlocked() {
  return receive_blocked_ && (!PrivateHasOutputData() || send_blocked_);
}

template<class Message>
void ProtocolConnectionImpl<Message>::SendAndReceive() {
  // TODO(dss): If output_data_ grows too large, temporarily suspend reads on
  // the socket until some of the output data can be sent. This will prevent
  // available memory from being exhausted.

  char input_buffer[1000];
  const ssize_t recv_count = recv(socket_fd_, input_buffer, sizeof input_buffer,
                                  0);

  if (recv_count > 0) {
    input_data_.append(input_buffer,
                       static_cast<std::string::size_type>(recv_count));
    ParseMessages(&input_data_);

    receive_blocked_ = false;
  } else {
    if (recv_count == 0) {
      VLOG(1) << "recv() returned 0";
      close_requested_ = true;
    } else {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        if (errno == ECONNRESET) {
          VLOG(1) << "recv() failed with ECONNRESET";
          close_requested_ = true;
        } else {
          PLOG(FATAL) << "recv";
        }
      }
    }

    receive_blocked_ = true;
  }

  if (PrivateHasOutputData()) {
    const ssize_t send_count = send(socket_fd_, output_data_.data(),
                                    output_data_.length(), MSG_NOSIGNAL);

    if (send_count >= 0) {
      output_data_.erase(0, static_cast<std::string::size_type>(send_count));

      send_blocked_ = false;
    } else {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        if (errno == ECONNRESET) {
          VLOG(1) << "send() failed with ECONNRESET";
          close_requested_ = true;
        } else if (errno == EPIPE) {
          VLOG(1) << "send() failed with EPIPE";
          close_requested_ = true;
        } else {
          PLOG(FATAL) << "send";
        }
      }

      send_blocked_ = true;
    }
  }
}

template<class Message>
void ProtocolConnectionImpl<Message>::CloseSocket() {
  CHECK_ERR(close(socket_fd_));
}

template<class Message>
void ProtocolConnectionImpl<Message>::Close() {
  close_requested_ = true;
}

template<class Message>
void ProtocolConnectionImpl<Message>::NotifyMessageReadyToSend() {
  protocol_server_->NotifyConnectionsChanged();
}

template<class Message>
void ProtocolConnectionImpl<Message>::ParseMessages(std::string* input_string) {
  CHECK(protocol_connection_handler_ != nullptr);
  CHECK(input_string != nullptr);

  for (;;) {
    Message message;
    const int char_count = ParseProtocolMessage(
        input_string->data(), static_cast<int>(input_string->length()),
        &message);

    if (char_count < 0) {
      return;
    }

    message.CheckInitialized();
    protocol_connection_handler_->NotifyMessageReceived(message);

    input_string->erase(0, static_cast<std::string::size_type>(char_count));
  }
}

template<class Message>
bool ProtocolConnectionImpl<Message>::PrivateHasOutputData() {
  CHECK(protocol_connection_handler_ != nullptr);

  if (output_data_.empty()) {
    Message message;

    if (protocol_connection_handler_->GetNextOutputMessage(&message)) {
      FormatProtocolMessage(message, &output_data_);
    }
  }

  return !output_data_.empty();
}

}  // namespace floating_temple

#endif  // PROTOCOL_SERVER_PROTOCOL_CONNECTION_IMPL_H_
