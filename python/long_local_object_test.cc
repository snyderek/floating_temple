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

#include "python/long_local_object.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <iostream>
#include <ostream>
#include <string>

#include "base/logging.h"
#include "python/python_gil_lock.h"
#include "python/python_scoped_ptr.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"
#include "third_party/gmock-1.7.0/include/gmock/gmock.h"

using google::InitGoogleLogging;
using std::ostream;
using std::string;
using testing::InitGoogleTest;
using testing::MakeMatcher;
using testing::MatchResultListener;
using testing::Matcher;
using testing::MatcherInterface;
using testing::Test;

namespace floating_temple {
namespace python {
namespace {

string PyObjectToAsciiString(PyObject* py_object) {
  CHECK(py_object != nullptr);

  const PythonScopedPtr py_ascii(PyObject_ASCII(py_object));
  const PythonScopedPtr py_bytes(PyUnicode_AsASCIIString(py_ascii.get()));

  char* buffer = nullptr;
  Py_ssize_t length = 0;
  CHECK_EQ(PyBytes_AsStringAndSize(py_bytes.get(), &buffer, &length), 0);
  CHECK_GE(length, 0);

  return string(buffer, static_cast<string::size_type>(length));
}

class SerializesCorrectlyMatcher : public MatcherInterface<PyObject*> {
 public:
  bool MatchAndExplain(PyObject* py_long,
                       MatchResultListener* listener) const override {
    CHECK(py_long != nullptr);
    CHECK_NE(PyLong_CheckExact(py_long), 0);

    // Serialize the long object.
    string bytes;
    SerializeLongObject(py_long, &bytes);

    // Deserialize the long object.
    PyObject* const py_deserialized_long = DeserializeLongObject(bytes);
    if (py_deserialized_long == nullptr) {
      *listener << "DeserializeLongObject returned NULL";
      return false;
    }

    // Use a string comparison to check that the deserialized long object is
    // equal to the original long object.
    const string ascii1 = PyObjectToAsciiString(py_long);
    const string ascii2 = PyObjectToAsciiString(py_deserialized_long);

    if (ascii1 != ascii2) {
      *listener << "original long object == " << ascii1
                << ", deserialized long object == " << ascii2;
      return false;
    }

    return true;
  }

  void DescribeTo(ostream* os) const override {
    *os << "serializes/deserializes correctly";
  }

  void DescribeNegationTo(ostream* os) const override {
    *os << "doesn't serialize/deserialize correctly";
  }
};

Matcher<PyObject*> SerializesCorrectly() {
  return MakeMatcher(new SerializesCorrectlyMatcher());
}

class LongLocalObjectTest : public Test {
 public:
  void SetUp() override {
    Py_InitializeEx(0);
    PyEval_InitThreads();
  }

  void TearDown() override {
    Py_Finalize();
  }
};

TEST_F(LongLocalObjectTest, Serialization) {
  EXPECT_THAT(PyLong_FromLong(0), SerializesCorrectly());
  EXPECT_THAT(PyLong_FromLong(1), SerializesCorrectly());
  EXPECT_THAT(PyLong_FromLong(-1), SerializesCorrectly());
  EXPECT_THAT(PyLong_FromLong(127), SerializesCorrectly());
  EXPECT_THAT(PyLong_FromLong(128), SerializesCorrectly());
  EXPECT_THAT(PyLong_FromLong(-128), SerializesCorrectly());
  EXPECT_THAT(PyLong_FromLongLong(9223372036854775807), SerializesCorrectly());
  EXPECT_THAT(PyLong_FromString("9223372036854775808", nullptr, 0),
              SerializesCorrectly());
  EXPECT_THAT(PyLong_FromString(
      "55555555555555555555555555555555555555555555555555555555555555555555555",
      nullptr, 0), SerializesCorrectly());
}

}  // namespace
}  // namespace python
}  // namespace floating_temple

int main(int argc, char** argv) {
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
