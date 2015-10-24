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

#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "include/c++/object_reference.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "lua/interpreter_impl.h"
#include "lua/third_party_lua_headers.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::string;
using std::vector;
using testing::AnyNumber;
using testing::InitGoogleMock;
using testing::_;

namespace floating_temple {
namespace lua {
namespace {

class MockThread : public Thread {
 public:
  MockThread() {}

  MOCK_METHOD0(BeginTransaction, bool());
  MOCK_METHOD0(EndTransaction, bool());
  MOCK_METHOD2(CreateVersionedObject,
               ObjectReference*(VersionedLocalObject* initial_version,
                                const string& name));
  MOCK_METHOD2(CreateUnversionedObject,
               ObjectReference*(UnversionedLocalObject* initial_version,
                                const string& name));
  MOCK_METHOD4(CallMethod,
               bool(ObjectReference* object_reference,
                    const string& method_name, const vector<Value>& parameters,
                    Value* return_value));
  MOCK_CONST_METHOD2(ObjectsAreIdentical,
                     bool(const ObjectReference* a, const ObjectReference* b));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockThread);
};

class MockObjectReference : public ObjectReference {
 public:
  MockObjectReference() {}

  MOCK_CONST_METHOD1(Dump, void(DumpContext* dc));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockObjectReference);
};

TEST(TableLocalObjectTest, SetTableAndGetTable) {
  InterpreterImpl interpreter;
  interpreter.Init();

  MockThread thread;

  EXPECT_CALL(thread, BeginTransaction())
      .Times(0);
  EXPECT_CALL(thread, EndTransaction())
      .Times(0);
  EXPECT_CALL(thread, CreateVersionedObject(_, _))
      .Times(0);
  EXPECT_CALL(thread, CreateUnversionedObject(_, _))
      .Times(0);
  EXPECT_CALL(thread, CallMethod(_, _, _, _))
      .Times(0);
  EXPECT_CALL(thread, ObjectsAreIdentical(_, _))
      .Times(0);

  TableLocalObject table_local_object(&interpreter);
  table_local_object.Init(0, 0);

  MockObjectReference table_object_reference;

  EXPECT_CALL(table_object_reference, Dump(_))
      .Times(AnyNumber());

  {
    vector<Value> parameters(2);
    parameters[0].set_string_value(LUA_TSTRING, "abc");
    parameters[1].set_double_value(LUA_TNUMBER, 123.45);

    Value return_value;
    table_local_object.InvokeMethod(&thread, &table_object_reference,
                                    "settable", parameters, &return_value);

    EXPECT_EQ(Value::EMPTY, return_value.type());
    EXPECT_EQ(LUA_TNIL, return_value.local_type());
  }

  {
    vector<Value> parameters(1);
    parameters[0].set_string_value(LUA_TSTRING, "abc");

    Value return_value;
    table_local_object.InvokeMethod(&thread, &table_object_reference,
                                    "gettable", parameters, &return_value);

    EXPECT_EQ(Value::DOUBLE, return_value.type());
    EXPECT_EQ(LUA_TNUMBER, return_value.local_type());
    ASSERT_DOUBLE_EQ(123.45, return_value.double_value());
  }
}

}  // namespace
}  // namespace lua
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}