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

#ifndef LUA_TABLE_LOCAL_OBJECT_H_
#define LUA_TABLE_LOCAL_OBJECT_H_

#include "base/macros.h"
#include "include/c++/versioned_local_object.h"
#include "third_party/lua-5.2.3/src/lobject.h"
#include "third_party/lua-5.2.3/src/lua.h"

namespace floating_temple {
namespace lua {

class TableLocalObject : public VersionedLocalObject {
 public:
  explicit TableLocalObject(lua_State* lua_state);
  ~TableLocalObject() override;

  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  VersionedLocalObject* Clone() const override;
  std::size_t Serialize(void* buffer, std::size_t buffer_size,
                        SerializationContext* context) const override;
  void Dump(DumpContext* dc) const override;

 private:
  lua_State* const lua_state_;
  TValue lua_table_;

  DISALLOW_COPY_AND_ASSIGN(TableLocalObject);
};

}  // namespace lua
}  // namespace floating_temple

#endif  // LUA_TABLE_LOCAL_OBJECT_H_
