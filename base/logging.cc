// Please don't add a copyright notice to this file. It contains source code
// owned by other copyright holders (used under license).

#include "base/logging.h"

#include <glog/logging.h>

using google::GLOG_FATAL;
using google::LogMessage;
using google::posix_strerror_r;

namespace floating_temple {

PthreadLogMessage::PthreadLogMessage(const char* file, int line,
                                     int error_number)
    : LogMessage(file, line, GLOG_FATAL),
      error_number_(error_number) {
}

PthreadLogMessage::~PthreadLogMessage() {
  // Different versions of glibc provide implementations of strerror_r that
  // behave differently and have different return types. The Google logging
  // library's posix_strerror_r function abstracts away these compatibility
  // issues.
  char buf[100];
  posix_strerror_r(error_number_, buf, sizeof buf);
  stream() << ": " << buf << " [" << error_number_ << "]";
}

}  // namespace floating_temple
