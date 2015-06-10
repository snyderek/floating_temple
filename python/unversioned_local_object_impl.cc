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

#include "python/unversioned_local_object_impl.h"

#include "third_party/Python-3.4.2/Include/Python.h"

#include <string>
#include <vector>

#include "base/escape.h"
#include "base/logging.h"
#include "include/c++/value.h"
#include "python/interpreter_impl.h"
#include "python/make_value.h"
#include "python/method_context.h"
#include "python/python_gil_lock.h"
#include "python/thread_substitution.h"

using std::string;
using std::vector;

namespace floating_temple {
namespace python {

UnversionedLocalObjectImpl::UnversionedLocalObjectImpl(PyObject* py_object)
    : py_object_(CHECK_NOTNULL(py_object)) {
}

UnversionedLocalObjectImpl::~UnversionedLocalObjectImpl() {
  PythonGilLock lock;
  Py_DECREF(py_object_);
}

#define CALL_TP_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->method)(py_object_), return_value); \
        return; \
      } \
    } while (false)

#define CALL_TP_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_TP_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_NB_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_number != nullptr); \
        CHECK(object_type->tp_as_number->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->tp_as_number->method)(py_object_), \
                        return_value); \
        return; \
      } \
    } while (false)

#define CALL_NB_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_number != nullptr); \
        CHECK(object_type->tp_as_number->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->tp_as_number->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_NB_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_number != nullptr); \
        CHECK(object_type->tp_as_number->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->tp_as_number->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_SQ_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_sequence != nullptr); \
        CHECK(object_type->tp_as_sequence->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->tp_as_sequence->method)(py_object_), \
                        return_value); \
        return; \
      } \
    } while (false)

#define CALL_SQ_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_sequence != nullptr); \
        CHECK(object_type->tp_as_sequence->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->tp_as_sequence->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_SQ_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_sequence != nullptr); \
        CHECK(object_type->tp_as_sequence->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->tp_as_sequence->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_MP_METHOD0(method) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_mapping != nullptr); \
        CHECK(object_type->tp_as_mapping->method != nullptr); \
        CHECK_EQ(parameters.size(), 0u); \
        MakeReturnValue((*object_type->tp_as_mapping->method)(py_object_), \
                        return_value); \
        return; \
      } \
    } while (false)

#define CALL_MP_METHOD1(method, param_type1) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_mapping != nullptr); \
        CHECK(object_type->tp_as_mapping->method != nullptr); \
        CHECK_EQ(parameters.size(), 1u); \
        MakeReturnValue( \
            (*object_type->tp_as_mapping->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

#define CALL_MP_METHOD2(method, param_type1, param_type2) \
    do { \
      if (method_name == #method) { \
        CHECK(object_type->tp_as_mapping != nullptr); \
        CHECK(object_type->tp_as_mapping->method != nullptr); \
        CHECK_EQ(parameters.size(), 2u); \
        MakeReturnValue( \
            (*object_type->tp_as_mapping->method)( \
                py_object_, \
                ExtractValue<param_type1>(parameters[0], &method_context), \
                ExtractValue<param_type2>(parameters[1], &method_context)), \
            return_value); \
        return; \
      } \
    } while (false)

// TODO(dss): This implementation of this method is duplicated in the
// VersionedLocalObjectImpl class. Factor out the duplicate code.
void UnversionedLocalObjectImpl::InvokeMethod(Thread* thread,
                                              PeerObject* peer_object,
                                              const string& method_name,
                                              const vector<Value>& parameters,
                                              Value* return_value) {
  CHECK(thread != nullptr);
  CHECK(peer_object != nullptr);

  VLOG(3) << "Invoke method on local object: " << method_name;

  MethodContext method_context;
  ThreadSubstitution thread_substitution(InterpreterImpl::instance(), thread);

  PythonGilLock lock;

  const PyTypeObject* const object_type = Py_TYPE(py_object_);
  CHECK(object_type != nullptr);

  // TODO(dss): Consider using binary search instead of linear search to find
  // the method given its name.

  // TODO(dss): Fail gracefully if the peer passes the wrong number of
  // parameters, or the wrong types of parameters.

  CALL_TP_METHOD1(tp_getattr, char*);
  CALL_TP_METHOD2(tp_setattr, char*, PyObject*);
  CALL_TP_METHOD0(tp_repr);
  CALL_TP_METHOD0(tp_hash);
  CALL_TP_METHOD2(tp_call, PyObject*, PyObject*);
  CALL_TP_METHOD0(tp_str);
  CALL_TP_METHOD1(tp_getattro, PyObject*);
  CALL_TP_METHOD2(tp_setattro, PyObject*, PyObject*);
  CALL_TP_METHOD2(tp_richcompare, PyObject*, int);
  CALL_TP_METHOD0(tp_iter);
  CALL_TP_METHOD0(tp_iternext);
  CALL_TP_METHOD2(tp_descr_get, PyObject*, PyObject*);
  CALL_TP_METHOD2(tp_descr_set, PyObject*, PyObject*);
  CALL_TP_METHOD2(tp_init, PyObject*, PyObject*);

  CALL_NB_METHOD1(nb_add, PyObject*);
  CALL_NB_METHOD1(nb_subtract, PyObject*);
  CALL_NB_METHOD1(nb_multiply, PyObject*);
  CALL_NB_METHOD1(nb_remainder, PyObject*);
  CALL_NB_METHOD1(nb_divmod, PyObject*);
  CALL_NB_METHOD2(nb_power, PyObject*, PyObject*);
  CALL_NB_METHOD0(nb_negative);
  CALL_NB_METHOD0(nb_positive);
  CALL_NB_METHOD0(nb_absolute);
  CALL_NB_METHOD0(nb_bool);
  CALL_NB_METHOD0(nb_invert);
  CALL_NB_METHOD1(nb_lshift, PyObject*);
  CALL_NB_METHOD1(nb_rshift, PyObject*);
  CALL_NB_METHOD1(nb_and, PyObject*);
  CALL_NB_METHOD1(nb_xor, PyObject*);
  CALL_NB_METHOD1(nb_or, PyObject*);
  CALL_NB_METHOD0(nb_int);
  CALL_NB_METHOD0(nb_float);
  CALL_NB_METHOD1(nb_inplace_add, PyObject*);
  CALL_NB_METHOD1(nb_inplace_subtract, PyObject*);
  CALL_NB_METHOD1(nb_inplace_multiply, PyObject*);
  CALL_NB_METHOD1(nb_inplace_remainder, PyObject*);
  CALL_NB_METHOD2(nb_inplace_power, PyObject*, PyObject*);
  CALL_NB_METHOD1(nb_inplace_lshift, PyObject*);
  CALL_NB_METHOD1(nb_inplace_rshift, PyObject*);
  CALL_NB_METHOD1(nb_inplace_and, PyObject*);
  CALL_NB_METHOD1(nb_inplace_xor, PyObject*);
  CALL_NB_METHOD1(nb_inplace_or, PyObject*);
  CALL_NB_METHOD1(nb_floor_divide, PyObject*);
  CALL_NB_METHOD1(nb_true_divide, PyObject*);
  CALL_NB_METHOD1(nb_inplace_floor_divide, PyObject*);
  CALL_NB_METHOD1(nb_inplace_true_divide, PyObject*);
  CALL_NB_METHOD0(nb_index);

  CALL_SQ_METHOD0(sq_length);
  CALL_SQ_METHOD1(sq_concat, PyObject*);
  CALL_SQ_METHOD1(sq_repeat, Py_ssize_t);
  CALL_SQ_METHOD1(sq_item, Py_ssize_t);
  CALL_SQ_METHOD2(sq_ass_item, Py_ssize_t, PyObject*);
  CALL_SQ_METHOD1(sq_contains, PyObject*);
  CALL_SQ_METHOD1(sq_inplace_concat, PyObject*);
  CALL_SQ_METHOD1(sq_inplace_repeat, Py_ssize_t);

  CALL_MP_METHOD0(mp_length);
  CALL_MP_METHOD1(mp_subscript, PyObject*);
  CALL_MP_METHOD2(mp_ass_subscript, PyObject*, PyObject*);

  // TODO(dss): Fail gracefully if a remote peer sends an invalid method name.
  LOG(FATAL) << "Unexpected method name \"" << CEscape(method_name) << "\"";
}

#undef CALL_TP_METHOD0
#undef CALL_TP_METHOD1
#undef CALL_TP_METHOD2
#undef CALL_NB_METHOD0
#undef CALL_NB_METHOD1
#undef CALL_NB_METHOD2
#undef CALL_SQ_METHOD0
#undef CALL_SQ_METHOD1
#undef CALL_SQ_METHOD2
#undef CALL_MP_METHOD0
#undef CALL_MP_METHOD1
#undef CALL_MP_METHOD2

}  // namespace python
}  // namespace floating_temple
