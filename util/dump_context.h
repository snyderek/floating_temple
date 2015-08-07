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

#ifndef UTIL_DUMP_CONTEXT_H_
#define UTIL_DUMP_CONTEXT_H_

#include <string>

#include "base/integral_types.h"

namespace floating_temple {

// An interface that eases the task of dumping the contents of a C++ object, for
// debugging purposes.
//
// The convention is that each dump-able class has a 'Dump' method that accepts
// a DumpContext pointer as a parameter. The Dump method calls various methods
// on the DumpContext object to output structured data.
class DumpContext {
 public:
  virtual ~DumpContext() {}

  // Each of these methods outputs a single value. To output multiple values,
  // first call BeginList or BeginMap.
  virtual void AddNull() = 0;
  virtual void AddBool(bool b) = 0;
  virtual void AddInt(int n) = 0;
  virtual void AddLong(long n) = 0;
  virtual void AddLongLong(long long n) = 0;
  virtual void AddInt64(int64 n) = 0;
  virtual void AddUint64(uint64 n) = 0;
  virtual void AddFloat(float f) = 0;
  virtual void AddDouble(double d) = 0;
  virtual void AddString(const std::string& s) = 0;
  virtual void AddPointer(const void* p) = 0;
  // TODO(dss): Add AddUuid and AddTransactionId methods.

  // Begins a list of values. Subsequent calls to the AddXxx methods will add
  // elements to the list. A list may also include nested lists and maps.
  virtual void BeginList() = 0;

  // Begins a map. Subsequent pairs of calls to the AddXxx methods will add
  // key/value pairs to the map. A nested list or map may be used as a map
  // value.
  virtual void BeginMap() = 0;

  // Terminates the most recent list or map.
  virtual void End() = 0;
};

}  // namespace floating_temple

#endif  // UTIL_DUMP_CONTEXT_H_
