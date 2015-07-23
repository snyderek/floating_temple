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

#ifndef ENGINE_TRANSACTION_ID_GENERATOR_H_
#define ENGINE_TRANSACTION_ID_GENERATOR_H_

#include "base/integral_types.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "engine/proto/uuid.pb.h"

namespace floating_temple {
namespace engine {

class TransactionId;

class TransactionIdGenerator {
 public:
  TransactionIdGenerator();

  void Generate(TransactionId* transaction_id);

 private:
  Uuid uuid_;
  uint64 last_time_value_;
  Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(TransactionIdGenerator);
};

}  // namespace engine
}  // namespace floating_temple

#endif  // ENGINE_TRANSACTION_ID_GENERATOR_H_
