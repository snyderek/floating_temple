// Floating Temple
// Copyright 2015 Derek S. Snyder
//
// $Id: convert_value.cc 3411 2015-02-11 07:13:45Z dss $

#include "lua/convert_value.h"

#include <cstddef>
#include <string>

#include "base/logging.h"
#include "include/c++/value.h"
#include "lua/interpreter_impl.h"
#include "third_party/lua-5.2.3/src/lobject.h"
#include "third_party/lua-5.2.3/src/lstring.h"
#include "third_party/lua-5.2.3/src/lua.h"

using std::string;

namespace floating_temple {

class PeerObject;

namespace lua {

void LuaValueToValue(const TValue* lua_value, Value* value) {
  CHECK(lua_value != NULL);
  CHECK(value != NULL);

  const int lua_type = ttypenv(lua_value);

  switch (lua_type) {
    case LUA_TNIL:
      value->set_empty(lua_type);
      break;

    case LUA_TBOOLEAN:
      value->set_bool_value(lua_type, (bvalue(lua_value) != 0));
      break;

    case LUA_TNUMBER:
      value->set_double_value(lua_type, nvalue(lua_value));
      break;

    case LUA_TSTRING: {
      const TString* const lua_string = rawtsvalue(lua_value);
      value->set_string_value(lua_type,
                              string(getstr(lua_string), lua_string->tsv.len));
      break;
    }

    case LUA_TPEEROBJECT:
      value->set_peer_object(
          lua_type,
          *reinterpret_cast<floating_temple::PeerObject* const*>(
              &val_(lua_value).po));
      break;

    default:
      LOG(FATAL) << "Unexpected lua value type: " << lua_type;
  }
}

void ValueToLuaValue(const Value& value, TValue* lua_value) {
  CHECK(lua_value != NULL);

  lua_State* const lua_state = InterpreterImpl::instance()->GetLuaState();
  const int lua_type = value.local_type();

  switch (lua_type) {
    case LUA_TNIL:
      setnilvalue(lua_value);
      break;

    case LUA_TBOOLEAN:
      setbvalue(lua_value, value.bool_value() ? 1 : 0);
      break;

    case LUA_TNUMBER:
      setnvalue(lua_value, value.double_value());
      break;

    case LUA_TSTRING: {
      const string& s = value.string_value();
      TString* const lua_string = luaS_newlstr(lua_state, s.data(), s.length());
      setsvalue(lua_state, lua_value, lua_string);
      break;
    }

    case LUA_TPEEROBJECT: {
      *reinterpret_cast<floating_temple::PeerObject**>(&val_(lua_value).po) =
          value.peer_object();
      settt_(lua_value, lua_type);
      break;
    }

    default:
      LOG(FATAL) << "Unexpected lua value type: " << lua_type;
  }
}

}  // namespace lua
}  // namespace floating_temple
