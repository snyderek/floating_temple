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

#include "lua/table_local_object.h"

#include <csetjmp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "include/c++/value.h"
#include "lua/convert_value.h"
#include "lua/interpreter_impl.h"
#include "lua/proto/serialization.pb.h"
#include "lua/third_party_lua_headers.h"
#include "lua/thread_substitution.h"
#include "util/dump_context.h"
#include "util/math_util.h"

using std::jmp_buf;
using std::size_t;
using std::string;
using std::unique_ptr;
using std::vector;

namespace floating_temple {
namespace lua {
namespace {

int GetTableNodeIndex(const Table* table, const Node* node) {
  return node - table->node;
}

// Since the following functions call setjmp(), they must not execute any
// destructors, either explicitly or implicitly.

bool InvokeMethod_GetTable(jmp_buf env, lua_State* lua_state,
                           const TValue* lua_table, TValue* lua_key,
                           TValue* lua_value) {
  if (setjmp(env) != 0) {
    return false;
  }

  luaV_gettable(lua_state, lua_table, lua_key, lua_value);
  return true;
}

bool InvokeMethod_SetTable(jmp_buf env, lua_State* lua_state,
                           const TValue* lua_table, TValue* lua_key,
                           TValue* lua_value) {
  if (setjmp(env) != 0) {
    return false;
  }

  luaV_settable(lua_state, lua_table, lua_key, lua_value);
  return true;
}

bool InvokeMethod_Len(jmp_buf env, lua_State* lua_state, TValue* lua_length,
                      const TValue* lua_table) {
  if (setjmp(env) != 0) {
    return false;
  }

  luaV_objlen(lua_state, lua_length, lua_table);
  return true;
}

bool InvokeMethod_SetList(jmp_buf env, lua_State* lua_state, TValue* lua_table,
                          int n, int c, TValue* lua_values) {
  if (setjmp(env) != 0) {
    return false;
  }

  // This code is mostly copy-pasted from the luaV_execute function in
  // "third_party/lua-5.2.3/src/lvm.c".
  Table* const h = hvalue(lua_table);
  int last = ((c - 1) * LFIELDS_PER_FLUSH) + n;

  if (last > h->sizearray) {
    luaH_resizearray(lua_state, h, last);
  }

  for (; n > 0; n--) {
    TValue* const lua_value = &lua_values[n - 1];
    luaH_setint(lua_state, h, last--, lua_value);
    luaC_barrierback(lua_state, obj2gco(h), lua_value);
  }

  return true;
}

}  // namespace

TableLocalObject::TableLocalObject(InterpreterImpl* interpreter)
    : interpreter_(CHECK_NOTNULL(interpreter)) {
}

TableLocalObject::~TableLocalObject() {
  if (lua_table_.get() != nullptr) {
    luaH_free(interpreter_->GetLuaState(), hvalue(lua_table_.get()));
  }
}

void TableLocalObject::Init(int b, int c) {
  CHECK(lua_table_.get() == nullptr)
      << "TableLocalObject::Init has already been called.";

  lua_State* const lua_state = interpreter_->GetLuaState();

  // This code is mostly copy-pasted from the luaH_new function in
  // "third_party/lua-5.2.3/src/ltable.c".
  GCObject* gc_list = nullptr;
  Table* const table =
      &luaC_newobj(lua_state, LUA_TTABLE, sizeof (Table), &gc_list, 0)->h;

  Node* const node = const_cast<Node*>(luaH_getdummynode());

  table->flags = static_cast<lu_byte>(~0);
  table->lsizenode = static_cast<lu_byte>(0);
  // TODO(dss): Support metatables.
  table->metatable = nullptr;
  table->array = nullptr;
  table->node = node;
  table->lastfree = node;
  table->gclist = nullptr;
  table->sizearray = 0;

  lua_table_.reset(new TValue());
  sethvalue(lua_state, lua_table_.get(), table);

  if (b != 0 || c != 0) {
    luaH_resize(lua_state, table, luaO_fb2int(b), luaO_fb2int(c));
  }
}

void TableLocalObject::InvokeMethod(Thread* thread,
                                    ObjectReference* object_reference,
                                    const string& method_name,
                                    const vector<Value>& parameters,
                                    Value* return_value) {
  CHECK(lua_table_.get() != nullptr)
      << "TableLocalObject::Init has not been called.";
  CHECK(return_value != nullptr);

  lua_State* const lua_state = interpreter_->GetLuaState();

  ThreadSubstitution thread_substitution(interpreter_, thread);

  InterpreterImpl::LongJumpTarget long_jump_target;
  interpreter_->SetLongJumpTarget(&long_jump_target);

  if (method_name == "gettable") {
    // TODO(dss): Fail gracefully if a remote peer sends a method with the wrong
    // number of parameters.
    CHECK_EQ(parameters.size(), 1u);

    TValue lua_key;
    ValueToLuaValue(lua_state, parameters[0], &lua_key);

    TValue lua_value;
    if (!InvokeMethod_GetTable(long_jump_target.env, lua_state,
                               lua_table_.get(), &lua_key, &lua_value)) {
      return;
    }

    LuaValueToValue(&lua_value, return_value);
  } else if (method_name == "settable") {
    CHECK_EQ(parameters.size(), 2u);

    TValue lua_key;
    ValueToLuaValue(lua_state, parameters[0], &lua_key);

    TValue lua_value;
    ValueToLuaValue(lua_state, parameters[1], &lua_value);

    if (!InvokeMethod_SetTable(long_jump_target.env, lua_state,
                               lua_table_.get(), &lua_key, &lua_value)) {
      return;
    }

    return_value->set_empty(LUA_TNIL);
  } else if (method_name == "len") {
    CHECK_EQ(parameters.size(), 0u);

    TValue lua_length;
    luaV_objlen(lua_state, &lua_length, lua_table_.get());
    if (!InvokeMethod_Len(long_jump_target.env, lua_state, &lua_length,
                          lua_table_.get())) {
      return;
    }

    LuaValueToValue(&lua_length, return_value);
  } else if (method_name == "setlist") {
    CHECK_GE(parameters.size(), 1u);

    const int n = static_cast<int>(parameters.size() - 1u);
    const int c = static_cast<int>(parameters[0].int64_value());

    unique_ptr<TValue[]> lua_values(new TValue[n]);
    for (int i = 1; i <= n; ++i) {
      ValueToLuaValue(lua_state, parameters[i], &lua_values[i - 1]);
    }

    if (!InvokeMethod_SetList(long_jump_target.env, lua_state, lua_table_.get(),
                              n, c, lua_values.get())) {
      return;
    }

    return_value->set_empty(LUA_TNIL);
  } else {
    // TODO(dss): Fail gracefully if a remote peer sends an invalid method name.
    LOG(FATAL) << "Unexpected method name \"" << CEscape(method_name) << "\"";
  }
}

VersionedLocalObject* TableLocalObject::Clone() const {
  CHECK(lua_table_.get() != nullptr)
      << "TableLocalObject::Init has not been called.";

  lua_State* const lua_state = interpreter_->GetLuaState();
  const Table* const old_table = hvalue(lua_table_.get());

  // TODO(dss): Move the code to clone a table into the third-party Lua source
  // code tree.

  GCObject* gc_list = nullptr;
  Table* const new_table =
      &luaC_newobj(lua_state, LUA_TTABLE, sizeof (Table), &gc_list, 0)->h;

  const lu_byte lsizenode = old_table->lsizenode;
  const int sizearray = old_table->sizearray;

  new_table->flags = old_table->flags;
  new_table->lsizenode = lsizenode;
  new_table->metatable = nullptr;

  if (old_table->array == nullptr) {
    new_table->array = nullptr;
  } else {
    new_table->array = luaM_newvector(lua_state, sizearray, TValue);
    for (int i = 0; i < sizearray; ++i) {
      setobj2t(&lua_state, &new_table->array[i], &old_table->array[i]);
    }
  }

  const Node* const dummy_node = luaH_getdummynode();

  if (old_table->node == dummy_node) {
    Node* const node = const_cast<Node*>(dummy_node);
    new_table->node = node;
    new_table->lastfree = node;
  } else {
    const int size = twoto(static_cast<int>(lsizenode));
    new_table->node = luaM_newvector(lua_state, size, Node);

    for (int i = 0; i < size; ++i) {
      const Node* const old_node = gnode(old_table, i);
      Node* const new_node = gnode(new_table, i);

      gnext(new_node) = gnode(new_table,
                              GetTableNodeIndex(old_table, gnext(old_node)));
      setobj2t(&lua_state, gkey(new_node), gkey(old_node));
      setobj2t(&lua_state, gval(new_node), gval(old_node));
    }

    new_table->lastfree = gnode(
        new_table, GetTableNodeIndex(old_table, old_table->lastfree));
  }

  new_table->gclist = nullptr;
  new_table->sizearray = sizearray;

  TableLocalObject* const new_local_object = new TableLocalObject(interpreter_);
  new_local_object->Init(new_table);

  return new_local_object;
}

size_t TableLocalObject::Serialize(void* buffer, size_t buffer_size,
                                   SerializationContext* context) const {
  CHECK(lua_table_.get() != nullptr)
      << "TableLocalObject::Init has not been called.";

  const Table* const table = hvalue(lua_table_.get());
  TableProto table_proto;

  // Store the array part of the table in the protocol buffer.
  if (table->array != nullptr) {
    const int sizearray = table->sizearray;
    ArrayProto* const array_proto = table_proto.mutable_array();

    for (int i = 0; i < sizearray; ++i) {
      TValueProto* const tvalue_proto =
          array_proto->add_element()->mutable_value();
      LuaValueToValueProto(&table->array[i], tvalue_proto, context);
    }
  }

  // Store the hashtable part of the table in the protocol buffer.
  if (table->node != luaH_getdummynode()) {
    const int size = twoto(static_cast<int>(table->lsizenode));
    HashtableProto* const hashtable_proto = table_proto.mutable_hashtable();

    hashtable_proto->set_size(size);

    for (int i = 0; i < size; ++i) {
      const Node* const node = &table->node[i];
      const TValue* const lua_key = gkey(node);
      const TValue* const lua_value = gval(node);

      if (!ttisnil(lua_key) || !ttisnil(lua_value)) {
        const Node* const next_node = gnext(node);
        HashtableNodeProto* const node_proto = hashtable_proto->add_node();

        node_proto->set_index(i);
        if (next_node != nullptr) {
          node_proto->set_next_index(GetTableNodeIndex(table, next_node));
        }
        LuaValueToValueProto(lua_key, node_proto->mutable_key(), context);
        LuaValueToValueProto(lua_value, node_proto->mutable_value(), context);
      }
    }

    hashtable_proto->set_last_free_index(GetTableNodeIndex(table,
                                                           table->lastfree));
  }

  // Serialize the protocol buffer to the output buffer.
  const size_t byte_size = static_cast<size_t>(table_proto.ByteSize());
  if (byte_size <= buffer_size) {
    table_proto.SerializeWithCachedSizesToArray(static_cast<uint8*>(buffer));
  }

  return byte_size;
}

void TableLocalObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  // TODO(dss): Consider adding more detail to the dump output (e.g., number of
  // keys in the table).
  dc->BeginMap();
  dc->AddString("type");
  dc->AddString("TableLocalObject");
  dc->End();
}

// static
TableLocalObject* TableLocalObject::Deserialize(
    InterpreterImpl* interpreter,
    const void* buffer,
    size_t buffer_size,
    DeserializationContext* context) {
  CHECK(interpreter != nullptr);
  CHECK(buffer != nullptr);

  lua_State* const lua_state = interpreter->GetLuaState();

  // Parse the protocol buffer from the input buffer.
  TableProto table_proto;
  CHECK(table_proto.ParseFromArray(buffer, buffer_size));

  // Allocate the Table struct.
  GCObject* gc_list = nullptr;
  Table* const table =
      &luaC_newobj(lua_state, LUA_TTABLE, sizeof (Table), &gc_list, 0)->h;

  table->flags = static_cast<lu_byte>(~0);
  table->metatable = nullptr;

  // Read the array part of the table from the protocol buffer.
  if (table_proto.has_array()) {
    const ArrayProto& array_proto = table_proto.array();
    const int sizearray = array_proto.element_size();

    table->array = luaM_newvector(lua_state, sizearray, TValue);

    for (int i = 0; i < sizearray; ++i) {
      const ArrayElementProto& element_proto = array_proto.element(i);
      ValueProtoToLuaValue(lua_state, element_proto.value(), &table->array[i],
                           context);
    }

    table->sizearray = sizearray;
  } else {
    table->array = nullptr;
    table->sizearray = 0;
  }

  // Read the hashtable part of the table from the protocol buffer.
  if (table_proto.has_hashtable()) {
    const HashtableProto& hashtable_proto = table_proto.hashtable();

    const int size = hashtable_proto.size();
    CHECK(IsPowerOfTwo(static_cast<unsigned>(size)))
        << size << " is not a power of two.";

    table->lsizenode = static_cast<lu_byte>(
        luaO_ceillog2(static_cast<unsigned>(size)));
    table->node = luaM_newvector(lua_state, size, Node);

    const int node_count = hashtable_proto.node_size();
    int prev_node_index = 0;

    for (int i = 0; i < node_count; ++i) {
      const HashtableNodeProto& node_proto = hashtable_proto.node(i);
      const int node_index = node_proto.index();

      // Initialize unused hashtable nodes.
      for (int j = prev_node_index + 1; j < node_index; ++j) {
        Node* const node = gnode(table, j);
        gnext(node) = nullptr;
        setnilvalue(gkey(node));
        setnilvalue(gval(node));
      }

      Node* const node = gnode(table, node_index);

      if (node_proto.has_next_index()) {
        gnext(node) = gnode(table, node_proto.next_index());
      } else {
        gnext(node) = nullptr;
      }
      ValueProtoToLuaValue(lua_state, node_proto.key(), gkey(node), context);
      ValueProtoToLuaValue(lua_state, node_proto.value(), gval(node), context);

      prev_node_index = node_index;
    }

    // Initialize unused hashtable nodes.
    for (int j = prev_node_index + 1; j < size; ++j) {
      Node* const node = gnode(table, j);
      gnext(node) = nullptr;
      setnilvalue(gkey(node));
      setnilvalue(gval(node));
    }

    table->lastfree = gnode(table, hashtable_proto.last_free_index());
  } else {
    table->lsizenode = static_cast<lu_byte>(0);
    Node* const node = const_cast<Node*>(luaH_getdummynode());
    table->node = node;
    table->lastfree = node;
  }

  table->gclist = nullptr;

  TableLocalObject* const new_local_object = new TableLocalObject(interpreter);
  new_local_object->Init(table);

  return new_local_object;
}

void TableLocalObject::Init(Table* table) {
  CHECK(lua_table_.get() == nullptr)
      << "TableLocalObject::Init has already been called.";
  CHECK(table != nullptr);

  lua_State* const lua_state = interpreter_->GetLuaState();
  // Suppress an "unused variable" warning if the sethvalue macro below doesn't
  // use lua_state.
  UNUSED(lua_state);

  lua_table_.reset(new TValue());
  sethvalue(lua_state, lua_table_.get(), table);
}

}  // namespace lua
}  // namespace floating_temple
