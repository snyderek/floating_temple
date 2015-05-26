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

#ifndef INCLUDE_CPP_UNVERSIONED_LOCAL_OBJECT_H_
#define INCLUDE_CPP_UNVERSIONED_LOCAL_OBJECT_H_

#include "include/c++/local_object.h"

namespace floating_temple {

// Subclasses of this class are thread-safe.
class UnversionedLocalObject : public LocalObject {
};

}  // namespace floating_temple

#endif  // INCLUDE_CPP_UNVERSIONED_LOCAL_OBJECT_H_
