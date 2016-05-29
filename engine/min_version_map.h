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

#ifndef ENGINE_MIN_VERSION_MAP_H_
#define ENGINE_MIN_VERSION_MAP_H_

#include "engine/transaction_id_util.h"
#include "engine/version_map.h"

namespace floating_temple {
namespace engine {

class TransactionId;

class TransactionIdLessThanFunction {
 public:
  bool operator()(const TransactionId& a, const TransactionId& b) const {
    return a < b;
  }
};

typedef VersionMap<TransactionIdLessThanFunction> MinVersionMap;

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_MIN_VERSION_MAP_H_
