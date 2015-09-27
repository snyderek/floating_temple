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

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "include/c++/value.h"
#include "lua/convert_value.h"
#include "lua/proto/serialization.pb.h"
#include "third_party/lua-5.2.3/src/lgc.h"
#include "third_party/lua-5.2.3/src/llimits.h"
#include "third_party/lua-5.2.3/src/lmem.h"
#include "third_party/lua-5.2.3/src/lobject.h"
#include "third_party/lua-5.2.3/src/lopcodes.h"
#include "third_party/lua-5.2.3/src/lstate.h"
#include "third_party/lua-5.2.3/src/ltable.h"
#include "third_party/lua-5.2.3/src/lua.h"
#include "third_party/lua-5.2.3/src/lvm.h"
#include "util/math_util.h"

using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {
namespace lua {
namespace {

int GetTableNodeIndex(const Table* table, const Node* node) {
  return node - table->node;
}

}  // namespace

TableLocalObject::TableLocalObject(lua_State* lua_state, int b, int c)
    : lua_state_(CHECK_NOTNULL(lua_state)),
      lua_table_(new TValue()) {
  // This code is mostly copy-pasted from the luaH_new function in
  // "third_party/lua-5.2.3/src/ltable.c".
  GCObject* gc_list = nullptr;
  Table* const table =
      &luaC_newobj(lua_state, LUA_TTABLE, sizeof (Table), &gc_list, 0)->h;

  Node* const node = const_cast<Node*>(luaH_getdummynode());

  table->flags = static_cast<lu_byte>(~0);
  table->lsizenode = static_cast<lu_byte>(~0);
  // TODO(dss): Support metatables.
  table->metatable = nullptr;
  table->array = nullptr;
  table->node = node;
  table->lastfree = node;
  table->gclist = nullptr;
  table->sizearray = 0;

  sethvalue(lua_state, lua_table_.get(), table);

  if (b != 0 || c != 0) {
    luaH_resize(lua_state, table, luaO_fb2int(b), luaO_fb2int(c));
  }
}

TableLocalObject::~TableLocalObject() {
  luaH_free(lua_state_, hvalue(lua_table_.get()));
}

void TableLocalObject::InvokeMethod(Thread* thread,
                                    ObjectReference* object_reference,
                                    const string& method_name,
                                    const vector<Value>& parameters,
                                    Value* return_value) {
  CHECK(return_value != nullptr);

  if (method_name == "gettable") {
    // TODO(dss): Fail gracefully if a remote peer sends a method with the wrong
    // number of parameters.
    CHECK_EQ(parameters.size(), 1u);

    TValue lua_key;
    ValueToLuaValue(lua_state_, parameters[0], &lua_key);

    TValue lua_value;
    luaV_gettable(lua_state_, lua_table_.get(), &lua_key, &lua_value);

    LuaValueToValue(&lua_value, return_value);
  } else if (method_name == "settable") {
    CHECK_EQ(parameters.size(), 2u);

    TValue lua_key;
    ValueToLuaValue(lua_state_, parameters[0], &lua_key);

    TValue lua_value;
    ValueToLuaValue(lua_state_, parameters[1], &lua_value);

    luaV_settable(lua_state_, lua_table_.get(), &lua_key, &lua_value);

    return_value->set_empty(LUA_TNIL);
  } else if (method_name == "len") {
    CHECK_EQ(parameters.size(), 0u);

    TValue lua_length;
    luaV_objlen(lua_state_, &lua_length, lua_table_.get());

    LuaValueToValue(&lua_length, return_value);
  } else if (method_name == "setlist") {
    CHECK_GE(parameters.size(), 1u);

    int n = static_cast<int>(parameters.size() - 1u);
    const int c = static_cast<int>(parameters[0].int64_value());

    // This code is mostly copy-pasted from the luaV_execute function in
    // "third_party/lua-5.2.3/src/lvm.c".
    Table* const h = hvalue(lua_table_.get());
    int last = ((c - 1) * LFIELDS_PER_FLUSH) + n;
    if (last > h->sizearray) {
      luaH_resizearray(lua_state_, h, last);
    }
    for (; n > 0; n--) {
      TValue lua_value;
      ValueToLuaValue(lua_state_, parameters[n], &lua_value);

      luaH_setint(lua_state_, h, last--, &lua_value);
      luaC_barrierback(lua_state_, obj2gco(h), &lua_value);
    }

    return_value->set_empty(LUA_TNIL);
  } else {
    // TODO(dss): Fail gracefully if a remote peer sends an invalid method name.
    LOG(FATAL) << "Unexpected method name \"" << CEscape(method_name) << "\"";
  }
}

VersionedLocalObject* TableLocalObject::Clone() const {
  const Table* const old_table = hvalue(lua_table_.get());

  GCObject* gc_list = nullptr;
  Table* const new_table =
      &luaC_newobj(lua_state_, LUA_TTABLE, sizeof (Table), &gc_list, 0)->h;

  const lu_byte lsizenode = old_table->lsizenode;
  const int sizearray = old_table->sizearray;

  new_table->flags = old_table->flags;
  new_table->lsizenode = lsizenode;
  new_table->metatable = nullptr;

  if (old_table->array == nullptr) {
    new_table->array = nullptr;
  } else {
    new_table->array = luaM_newvector(lua_state_, sizearray, TValue);
    for (int i = 0; i < sizearray; ++i) {
      setobj2t(&lua_state_, &new_table->array[i], &old_table->array[i]);
    }
  }

  const Node* const dummy_node = luaH_getdummynode();

  if (old_table->node == dummy_node) {
    Node* const node = const_cast<Node*>(dummy_node);
    new_table->node = node;
    new_table->lastfree = node;
  } else {
    const int size = twoto(static_cast<int>(lsizenode));
    new_table->node = luaM_newvector(lua_state_, size, Node);

    for (int i = 0; i < size; ++i) {
      const Node* const old_node = gnode(old_table, i);
      Node* const new_node = gnode(new_table, i);

      gnext(new_node) = gnode(new_table,
                              GetTableNodeIndex(old_table, gnext(old_node)));
      setobj2t(&lua_state_, gkey(new_node), gkey(old_node));
      setobj2t(&lua_state_, gval(new_node), gval(old_node));
    }

    new_table->lastfree = gnode(
        new_table, GetTableNodeIndex(old_table, old_table->lastfree));
  }

  new_table->gclist = nullptr;
  new_table->sizearray = sizearray;

  return new TableLocalObject(lua_state_, new_table);
}

size_t TableLocalObject::Serialize(void* buffer, size_t buffer_size,
                                   SerializationContext* context) const {
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

  // TODO(dss): Implement this.
}

// static
TableLocalObject* TableLocalObject::Deserialize(
    lua_State* lua_state,
    const void* buffer,
    size_t buffer_size,
    DeserializationContext* context) {
  CHECK(lua_state != nullptr);
  CHECK(buffer != nullptr);

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
    table->lsizenode = static_cast<lu_byte>(~0);
    Node* const node = const_cast<Node*>(luaH_getdummynode());
    table->node = node;
    table->lastfree = node;
  }

  table->gclist = nullptr;

  return new TableLocalObject(lua_state, table);
}

TableLocalObject::TableLocalObject(lua_State* lua_state, Table* table)
    : lua_state_(CHECK_NOTNULL(lua_state)),
      lua_table_(new TValue()) {
  CHECK(table != nullptr);
  sethvalue(lua_state, lua_table_.get(), table);
}

}  // namespace lua
}  // namespace floating_temple
