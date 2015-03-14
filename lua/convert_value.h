// Floating Temple
// Copyright 2015 Derek S. Snyder
//
// $Id: convert_value.h 3411 2015-02-11 07:13:45Z dss $

#include "third_party/lua-5.2.3/src/lobject.h"

namespace floating_temple {

class Value;

namespace lua {

void LuaValueToValue(const TValue* lua_value, Value* value);
void ValueToLuaValue(const Value& value, TValue* lua_value);

}  // namespace lua
}  // namespace floating_temple
