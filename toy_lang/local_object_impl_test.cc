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

#include "toy_lang/local_object_impl.h"

#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "base/macros.h"
#include "include/c++/object_reference.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"
#include "toy_lang/expression.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::shared_ptr;
using std::string;
using std::vector;
using testing::InitGoogleMock;
using testing::Return;
using testing::_;

namespace floating_temple {
namespace toy_lang {
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

  MOCK_CONST_METHOD0(Dump, string());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockObjectReference);
};

TEST(LocalObjectImplTest, InvokeMethodOnExpressionObject) {
  MockThread thread;
  MockObjectReference symbol_table_object_reference;

  EXPECT_CALL(thread, BeginTransaction())
      .Times(0);
  EXPECT_CALL(thread, EndTransaction())
      .Times(0);
  EXPECT_CALL(thread, CreateVersionedObject(_, _))
      .Times(0);
  EXPECT_CALL(thread, CreateUnversionedObject(_, _))
      .Times(0);
  EXPECT_CALL(thread, ObjectsAreIdentical(_, _))
      .Times(0);

  EXPECT_CALL(symbol_table_object_reference, Dump())
      .WillRepeatedly(Return(""));

  // Simulate Thread::CallMethod returning false. This should cause
  // ExpressionObject::InvokeMethod (called below) to immediately return.
  EXPECT_CALL(thread, CallMethod(&symbol_table_object_reference, "get", _, _))
      .WillRepeatedly(Return(false));

  ExpressionObject expression_local_object(
      shared_ptr<const Expression>(new VariableExpression("some_variable")));

  MockObjectReference expression_object_reference;

  EXPECT_CALL(expression_object_reference, Dump())
      .WillRepeatedly(Return(""));

  vector<Value> parameters(1);
  parameters[0].set_object_reference(0, &symbol_table_object_reference);

  Value return_value;
  expression_local_object.InvokeMethod(&thread, &expression_object_reference,
                                       "eval", parameters, &return_value);
}

}  // namespace
}  // namespace toy_lang
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
