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

#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "base/const_shared_ptr.h"
#include "base/logging.h"
#include "base/macros.h"
#include "include/c++/peer_object.h"
#include "include/c++/thread.h"
#include "include/c++/value.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"
#include "toy_lang/expression.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
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
  MOCK_METHOD3(CreatePeerObject,
               PeerObject*(VersionedLocalObject* initial_version,
                           const string& name, bool versioned));
  MOCK_METHOD4(CallMethod,
               bool(PeerObject* peer_object, const string& method_name,
                    const vector<Value>& parameters, Value* return_value));
  MOCK_CONST_METHOD2(ObjectsAreEquivalent,
                     bool(const PeerObject* a, const PeerObject* b));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockThread);
};

class MockPeerObject : public PeerObject {
 public:
  MockPeerObject() {}

  MOCK_CONST_METHOD0(Dump, string());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPeerObject);
};

TEST(LocalObjectImplTest, InvokeMethodOnExpressionObject) {
  MockThread thread;
  MockPeerObject symbol_table_peer_object;

  EXPECT_CALL(thread, BeginTransaction())
      .Times(0);
  EXPECT_CALL(thread, EndTransaction())
      .Times(0);
  EXPECT_CALL(thread, CreatePeerObject(_, _, _))
      .Times(0);
  EXPECT_CALL(thread, ObjectsAreEquivalent(_, _))
      .Times(0);

  EXPECT_CALL(symbol_table_peer_object, Dump())
      .WillRepeatedly(Return(""));

  // Simulate Thread::CallMethod returning false. This should cause
  // ExpressionObject::InvokeMethod (called below) to immediately return.
  EXPECT_CALL(thread, CallMethod(&symbol_table_peer_object, "get", _, _))
      .WillRepeatedly(Return(false));

  ExpressionObject expression_local_object(
      const_shared_ptr<Expression>(new VariableExpression("some_variable")));

  MockPeerObject expression_peer_object;

  EXPECT_CALL(expression_peer_object, Dump())
      .WillRepeatedly(Return(""));

  vector<Value> parameters(1);
  parameters[0].set_peer_object(0, &symbol_table_peer_object);

  Value return_value;
  expression_local_object.InvokeMethod(&thread, &expression_peer_object, "eval",
                                       parameters, &return_value);
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
