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

#include "base/string_printf.h"

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>

#include "base/logging.h"

using std::size_t;
using std::string;
using std::unique_ptr;
using std::va_list;
using std::vsnprintf;

namespace floating_temple {

string StringPrintf(const char* format, ...) {
  va_list arg_list;
  va_start(arg_list, format);
  const string str = StringPrintfV(format, arg_list);
  va_end(arg_list);
  return str;
}

void SStringPrintf(string* str, const char* format, ...) {
  va_list arg_list;
  va_start(arg_list, format);
  SStringPrintfV(str, format, arg_list);
  va_end(arg_list);
}

void StringAppendF(string* str, const char* format, ...) {
  va_list arg_list;
  va_start(arg_list, format);
  StringAppendFV(str, format, arg_list);
  va_end(arg_list);
}

string StringPrintfV(const char* format, va_list arg_list) {
  string str;
  StringAppendFV(&str, format, arg_list);
  return str;
}

void SStringPrintfV(string* str, const char* format, va_list arg_list) {
  str->clear();
  StringAppendFV(str, format, arg_list);
}

void StringAppendFV(string* str, const char* format, va_list arg_list) {
  CHECK(str != NULL);
  CHECK(format != NULL);

  va_list arg_list_copy;
  va_copy(arg_list_copy, arg_list);

  // Allocate a fixed-size buffer to hold the result string. This should be
  // large enough most of the time.
  char static_buffer[1000];
  size_t buffer_size = sizeof static_buffer;
  char* buffer = static_buffer;

  int count = vsnprintf(buffer, buffer_size, format, arg_list);
  CHECK_GE(count, 0);

  // If the fixed-size buffer was too small, dynamically allocate a buffer of
  // the required size and call vsnprintf again.
  unique_ptr<char[]> dynamic_buffer;

  if (static_cast<size_t>(count) >= buffer_size) {
    buffer_size = static_cast<size_t>(count + 1);
    dynamic_buffer.reset(new char[buffer_size]);
    buffer = dynamic_buffer.get();

    count = vsnprintf(buffer, buffer_size, format, arg_list_copy);
    CHECK_EQ(static_cast<size_t>(count + 1), buffer_size);
  }

  // Copy the contents of the buffer to the string object.
  str->append(buffer, static_cast<string::size_type>(count));

  va_end(arg_list_copy);
}

}  // namespace floating_temple
