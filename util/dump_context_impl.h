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

#ifndef UTIL_DUMP_CONTEXT_IMPL_H_
#define UTIL_DUMP_CONTEXT_IMPL_H_

#include <memory>
#include <stack>
#include <string>

#include "base/macros.h"
#include "util/dump_context.h"

namespace floating_temple {

class DumpContextImpl : public DumpContext {
 public:
  DumpContextImpl();
  ~DumpContextImpl() override;

  void FormatJson(std::string* output) const;

  void AddBool(bool b) override;
  void AddInt(int n) override;
  void AddLong(long n) override;
  void AddLongLong(long long n) override;
  void AddInt64(int64 n) override;
  void AddUint64(uint64 n) override;
  void AddFloat(float f) override;
  void AddDouble(double d) override;
  void AddString(const std::string& s) override;
  void AddPointer(const void* p) override;
  void BeginList() override;
  void BeginMap() override;
  void End() override;

 private:
  class DumpNode;
  class BoolDumpNode;
  class IntDumpNode;
  class LongDumpNode;
  class LongLongDumpNode;
  class Int64DumpNode;
  class Uint64DumpNode;
  class FloatDumpNode;
  class DoubleDumpNode;
  class StringDumpNode;
  class PointerDumpNode;
  class ListDumpNode;
  class MapDumpNode;

  void AddValue(DumpNode* node);

  std::stack<std::unique_ptr<DumpNode>> pending_nodes_;
  std::unique_ptr<DumpNode> root_node_;

  DISALLOW_COPY_AND_ASSIGN(DumpContextImpl);
};

}  // namespace floating_temple

#endif  // UTIL_DUMP_CONTEXT_IMPL_H_
