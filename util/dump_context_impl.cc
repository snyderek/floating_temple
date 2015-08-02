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

#include "util/dump_context_impl.h"

#include <cinttypes>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/string_printf.h"

using std::string;
using std::unique_ptr;
using std::vector;

namespace floating_temple {
namespace {

constexpr char kIntPrintfFormat[] = "%d";
constexpr char kLongPrintfFormat[] = "%ld";
constexpr char kLongLongPrintfFormat[] = "%lld";
constexpr char kInt64PrintfFormat[] = "%" PRId64;
constexpr char kUint64PrintfFormat[] = "%" PRIu64;
constexpr char kDoublePrintfFormat[] = "%f";

}  // namespace

class DumpContextImpl::DumpNode {
 public:
  virtual ~DumpNode() {}

  virtual void AddValue(DumpNode* node);
  virtual void AppendJson(string* output) const = 0;
};

class DumpContextImpl::NullDumpNode : public DumpContextImpl::DumpNode {
 public:
  NullDumpNode();

  void AppendJson(string* output) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NullDumpNode);
};

class DumpContextImpl::BoolDumpNode : public DumpContextImpl::DumpNode {
 public:
  explicit BoolDumpNode(bool b);

  void AppendJson(string* output) const override;

 private:
  const bool b_;

  DISALLOW_COPY_AND_ASSIGN(BoolDumpNode);
};

class DumpContextImpl::StringDumpNode : public DumpContextImpl::DumpNode {
 public:
  explicit StringDumpNode(const string& s);

  void AppendJson(string* output) const override;

 private:
  const string s_;

  DISALLOW_COPY_AND_ASSIGN(StringDumpNode);
};

class DumpContextImpl::PointerDumpNode : public DumpContextImpl::DumpNode {
 public:
  explicit PointerDumpNode(const void* p);

  void AppendJson(string* output) const override;

 private:
  const void* const p_;

  DISALLOW_COPY_AND_ASSIGN(PointerDumpNode);
};

class DumpContextImpl::ListDumpNode : public DumpContextImpl::DumpNode {
 public:
  ListDumpNode();

  void AddValue(DumpNode* node) override;
  void AppendJson(string* output) const override;

 private:
  vector<unique_ptr<DumpNode>> list_;

  DISALLOW_COPY_AND_ASSIGN(ListDumpNode);
};

class DumpContextImpl::MapDumpNode : public DumpContextImpl::DumpNode {
 public:
  MapDumpNode();

  void AddValue(DumpNode* node) override;
  void AppendJson(string* output) const override;

 private:
  // Preserve the order of the map items.
  vector<unique_ptr<DumpNode>> map_;

  DISALLOW_COPY_AND_ASSIGN(MapDumpNode);
};

template<typename T, const char* printf_format>
class DumpContextImpl::PrimitiveTypeDumpNode
    : public DumpContextImpl::DumpNode {
 public:
  explicit PrimitiveTypeDumpNode(T value);

  void AppendJson(string* output) const override;

 private:
  const T value_;

  DISALLOW_COPY_AND_ASSIGN(PrimitiveTypeDumpNode);
};

template<typename T, const char* printf_format>
DumpContextImpl::PrimitiveTypeDumpNode<T, printf_format>::PrimitiveTypeDumpNode(
    T value)
    : value_(value) {
}

template<typename T, const char* printf_format>
void DumpContextImpl::PrimitiveTypeDumpNode<T, printf_format>::AppendJson(
    string* output) const {
  StringAppendF(output, printf_format, value_);
}

DumpContextImpl::DumpContextImpl() {
}

DumpContextImpl::~DumpContextImpl() {
}

void DumpContextImpl::FormatJson(string* output) const {
  CHECK(root_node_.get() != nullptr);
  CHECK(output != nullptr);

  output->clear();
  root_node_->AppendJson(output);
}

void DumpContextImpl::AddNull() {
  AddValue(new NullDumpNode());
}

void DumpContextImpl::AddBool(bool b) {
  AddValue(new BoolDumpNode(b));
}

void DumpContextImpl::AddInt(int n) {
  AddValue(new PrimitiveTypeDumpNode<int, kIntPrintfFormat>(n));
}

void DumpContextImpl::AddLong(long n) {
  AddValue(new PrimitiveTypeDumpNode<long, kLongPrintfFormat>(n));
}

void DumpContextImpl::AddLongLong(long long n) {
  AddValue(new PrimitiveTypeDumpNode<long long, kLongLongPrintfFormat>(n));
}

void DumpContextImpl::AddInt64(int64 n) {
  AddValue(new PrimitiveTypeDumpNode<int64, kInt64PrintfFormat>(n));
}

void DumpContextImpl::AddUint64(uint64 n) {
  AddValue(new PrimitiveTypeDumpNode<uint64, kUint64PrintfFormat>(n));
}

void DumpContextImpl::AddFloat(float f) {
  const double d = static_cast<double>(f);
  AddValue(new PrimitiveTypeDumpNode<double, kDoublePrintfFormat>(d));
}

void DumpContextImpl::AddDouble(double d) {
  AddValue(new PrimitiveTypeDumpNode<double, kDoublePrintfFormat>(d));
}

void DumpContextImpl::AddString(const string& s) {
  AddValue(new StringDumpNode(s));
}

void DumpContextImpl::AddPointer(const void* p) {
  AddValue(new PointerDumpNode(p));
}

void DumpContextImpl::BeginList() {
  pending_nodes_.emplace(new ListDumpNode());
}

void DumpContextImpl::BeginMap() {
  pending_nodes_.emplace(new MapDumpNode());
}

void DumpContextImpl::End() {
  CHECK(!pending_nodes_.empty());

  DumpNode* const completed_node = pending_nodes_.top().release();
  pending_nodes_.pop();
  AddValue(completed_node);
}

void DumpContextImpl::AddValue(DumpNode* node) {
  if (pending_nodes_.empty()) {
    CHECK(root_node_.get() == nullptr);
    root_node_.reset(node);
  } else {
    pending_nodes_.top()->AddValue(node);
  }
}

void DumpContextImpl::DumpNode::AddValue(DumpNode* node) {
  LOG(FATAL) << "This node type does not support adding values.";
}

DumpContextImpl::NullDumpNode::NullDumpNode() {
}

void DumpContextImpl::NullDumpNode::AppendJson(string* output) const {
  *output += "null";
}

DumpContextImpl::BoolDumpNode::BoolDumpNode(bool b)
    : b_(b) {
}

void DumpContextImpl::BoolDumpNode::AppendJson(string* output) const {
  *output += (b_ ? "true" : "false");
}

DumpContextImpl::StringDumpNode::StringDumpNode(const string& s)
    : s_(s) {
}

void DumpContextImpl::StringDumpNode::AppendJson(string* output) const {
  StringAppendF(output, "\"%s\"", CEscape(s_).c_str());
}

DumpContextImpl::PointerDumpNode::PointerDumpNode(const void* p)
    : p_(p) {
}

void DumpContextImpl::PointerDumpNode::AppendJson(string* output) const {
  StringAppendF(output, "\"%p\"", p_);
}

DumpContextImpl::ListDumpNode::ListDumpNode() {
}

void DumpContextImpl::ListDumpNode::AddValue(DumpNode* node) {
  list_.emplace_back(node);
}

void DumpContextImpl::ListDumpNode::AppendJson(string* output) const {
  if (list_.empty()) {
    *output += "[]";
  } else {
    *output += "[";

    bool first = true;
    for (const unique_ptr<DumpNode>& node : list_) {
      if (first) {
        first = false;
      } else {
        *output += ',';
      }

      *output += ' ';
      node->AppendJson(output);
    }
    *output += " ]";
  }
}

DumpContextImpl::MapDumpNode::MapDumpNode() {
}

void DumpContextImpl::MapDumpNode::AddValue(DumpNode* node) {
  map_.emplace_back(node);
}

void DumpContextImpl::MapDumpNode::AppendJson(string* output) const {
  if (map_.empty()) {
    *output += "{}";
  } else {
    *output += "{";

    const auto begin_it = map_.begin();
    const auto end_it = map_.end();

    for (auto it = begin_it; it != end_it; ++it) {
      if (it != begin_it) {
        *output += ',';
      }

      *output += ' ';
      const DumpNode* const key = it->get();
      key->AppendJson(output);
      *output += ": ";

      ++it;
      CHECK(it != end_it);
      const DumpNode* const value = it->get();
      value->AppendJson(output);
    }
    *output += " }";
  }
}

}  // namespace floating_temple
