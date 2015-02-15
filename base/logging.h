#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <glog/logging.h>

#include "base/macros.h"

// Invokes the specified Pthreads function call. If the call returns an error,
// this macro logs a FATAL error that contains the error number and the error
// message.
//
// This macro is based on the CHECK_ERR macro defined in the Google logging
// library. CHECK_ERR can't be used with Pthreads functions because, unlike
// other system functions, the Pthreads functions report error codes by
// returning them, rather than by setting the errno variable.
#define CHECK_PTHREAD_ERR(invocation) \
  while (int error_number = (invocation)) \
    floating_temple::PthreadLogMessage(__FILE__, __LINE__, \
                                       error_number).stream() \
        << #invocation

namespace floating_temple {

using google::ERROR;
using google::FATAL;
using google::INFO;
using google::WARNING;

class PthreadLogMessage : public google::LogMessage {
 public:
  PthreadLogMessage(const char* file, int line, int error_number);
  ~PthreadLogMessage();

 private:
  const int error_number_;

  DISALLOW_COPY_AND_ASSIGN(PthreadLogMessage);
};

}  // namespace floating_temple

#endif  // BASE_LOGGING_H_
