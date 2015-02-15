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
  char buf[100];
  posix_strerror_r(error_number_, buf, sizeof buf);
  stream() << ": " << buf << " [" << error_number_ << "]";
}

}  // namespace floating_temple
