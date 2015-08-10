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
#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "include/c++/value.h"
#include "lua/convert_value.h"
#include "third_party/lua-5.2.3/src/lgc.h"
#include "third_party/lua-5.2.3/src/lobject.h"
#include "third_party/lua-5.2.3/src/lopcodes.h"
#include "third_party/lua-5.2.3/src/lstate.h"
#include "third_party/lua-5.2.3/src/ltable.h"
#include "third_party/lua-5.2.3/src/lua.h"
#include "third_party/lua-5.2.3/src/lvm.h"

using std::size_t;
using std::string;
using std::vector;

namespace floating_temple {
namespace lua {

TableLocalObject::TableLocalObject(lua_State* lua_state)
    : lua_state_(CHECK_NOTNULL(lua_state)) {
  Table* const table = luaH_new_nogc(lua_state);
  CHECK(table != nullptr);
  sethvalue(lua_state, &lua_table_, table);
}

TableLocalObject::~TableLocalObject() {
  luaH_free(lua_state_, hvalue(&lua_table_));
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
    ValueToLuaValue(parameters[0], &lua_key);

    TValue lua_value;
    luaV_gettable(lua_state_, &lua_table_, &lua_key, &lua_value);

    LuaValueToValue(&lua_value, return_value);
  } else if (method_name == "settable") {
    CHECK_EQ(parameters.size(), 2u);

    TValue lua_key;
    ValueToLuaValue(parameters[0], &lua_key);

    TValue lua_value;
    ValueToLuaValue(parameters[1], &lua_value);

    luaV_settable(lua_state_, &lua_table_, &lua_key, &lua_value);

    return_value->set_empty(LUA_TNIL);
  } else if (method_name == "len") {
    CHECK_EQ(parameters.size(), 0u);

    TValue lua_length;
    luaV_objlen(lua_state_, &lua_length, &lua_table_);

    LuaValueToValue(&lua_length, return_value);
  } else if (method_name == "setlist") {
    CHECK_EQ(parameters.size(), 2u);

    int n = static_cast<int>(parameters[0].int64_value());
    const int c = static_cast<int>(parameters[1].int64_value());

    // This code is mostly copy-pasted from the luaV_execute function in
    // "third_party/lua-5.2.3/src/lvm.c".
    Table* const h = hvalue(&lua_table_);
    int last = ((c - 1) * LFIELDS_PER_FLUSH) + n;
    if (last > h->sizearray) {
      luaH_resizearray(lua_state_, h, last);
    }
    for (; n > 0; n--) {
      TValue* const val = &lua_table_ + n;
      luaH_setint(lua_state_, h, last--, val);
      luaC_barrierback(lua_state_, obj2gco(h), val);
    }

    return_value->set_empty(LUA_TNIL);
  } else {
    // TODO(dss): Fail gracefully if a remote peer sends an invalid method name.
    LOG(FATAL) << "Unexpected method name \"" << CEscape(method_name) << "\"";
  }
}

VersionedLocalObject* TableLocalObject::Clone() const {
  // TODO(dss): Implement this.
  return nullptr;
}

size_t TableLocalObject::Serialize(void* buffer, size_t buffer_size,
                                   SerializationContext* context) const {
  // TODO(dss): Implement this.
  return 0;
}

void TableLocalObject::Dump(DumpContext* dc) const {
  CHECK(dc != nullptr);

  // TODO(dss): Implement this.
}

}  // namespace lua
}  // namespace floating_temple
