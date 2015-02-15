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

#include "toy_lang/interpreter_impl.h"

#include <cstddef>

#include "toy_lang/local_object_impl.h"

using std::size_t;

namespace floating_temple {
namespace toy_lang {

InterpreterImpl::InterpreterImpl() {
}

InterpreterImpl::~InterpreterImpl() {
}

LocalObject* InterpreterImpl::DeserializeObject(
    const void* buffer, size_t buffer_size, DeserializationContext* context) {
  return LocalObjectImpl::Deserialize(buffer, buffer_size, context);
}

}  // namespace toy_lang
}  // namespace floating_temple
