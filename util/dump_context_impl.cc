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

#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/string_printf.h"

using std::string;
using std::unique_ptr;

namespace floating_temple {

class DumpContextImpl::DumpNode {
 public:
  virtual ~DumpNode() {}

  virtual void AddValue(DumpNode* node);
  virtual void AppendJson(std::string* output) const = 0;
};

class DumpContextImpl::BoolDumpNode : public DumpContextImpl::DumpNode {
 public:
  explicit BoolDumpNode(bool b);

  void AppendJson(std::string* output) const override;

 private:
  const bool b_;

  DISALLOW_COPY_AND_ASSIGN(BoolDumpNode);
};

class DumpContextImpl::IntDumpNode : public DumpContextImpl::DumpNode {
 public:
  explicit IntDumpNode(int n);

  void AppendJson(std::string* output) const override;

 private:
  const int n_;

  DISALLOW_COPY_AND_ASSIGN(IntDumpNode);
};

class DumpContextImpl::StringDumpNode : public DumpContextImpl::DumpNode {
 public:
  explicit StringDumpNode(const std::string& s);

  void AppendJson(std::string* output) const override;

 private:
  const std::string s_;

  DISALLOW_COPY_AND_ASSIGN(StringDumpNode);
};

class DumpContextImpl::PointerDumpNode : public DumpContextImpl::DumpNode {
 public:
  explicit PointerDumpNode(const void* p);

  void AppendJson(std::string* output) const override;

 private:
  const void* const p_;

  DISALLOW_COPY_AND_ASSIGN(PointerDumpNode);
};

class DumpContextImpl::ListDumpNode : public DumpContextImpl::DumpNode {
 public:
  ListDumpNode();

  void AddValue(DumpNode* node) override;
  void AppendJson(std::string* output) const override;

 private:
  std::vector<std::unique_ptr<DumpNode>> list_;

  DISALLOW_COPY_AND_ASSIGN(ListDumpNode);
};

class DumpContextImpl::MapDumpNode : public DumpContextImpl::DumpNode {
 public:
  MapDumpNode();

  void AddValue(DumpNode* node) override;
  void AppendJson(std::string* output) const override;

 private:
  // Preserve the order of the map items.
  std::vector<std::unique_ptr<DumpNode>> map_;

  DISALLOW_COPY_AND_ASSIGN(MapDumpNode);
};

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

void DumpContextImpl::AddBool(bool b) {
  AddValue(new BoolDumpNode(b));
}

void DumpContextImpl::AddInt(int n) {
  AddValue(new IntDumpNode(n));
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

  if (pending_nodes_.empty()) {
    CHECK(root_node_.get() == nullptr);
    root_node_.reset(completed_node);
  } else {
    AddValue(completed_node);
  }
}

void DumpContextImpl::AddValue(DumpNode* node) {
  CHECK(!pending_nodes_.empty());
  pending_nodes_.top()->AddValue(node);
}

void DumpContextImpl::DumpNode::AddValue(DumpNode* node) {
  LOG(FATAL) << "This node type does not support adding values.";
}

DumpContextImpl::BoolDumpNode::BoolDumpNode(bool b)
    : b_(b) {
}

void DumpContextImpl::BoolDumpNode::AppendJson(string* output) const {
  *output += (b_ ? "true" : "false");
}

DumpContextImpl::IntDumpNode::IntDumpNode(int n)
    : n_(n) {
}

void DumpContextImpl::IntDumpNode::AppendJson(string* output) const {
  StringAppendF(output, "%d", n_);
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
