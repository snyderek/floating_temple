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

#include <cstdio>
#include <string>

#include "base/logging.h"
#include "engine/proto/uuid.pb.h"
#include "engine/uuid_util.h"

using floating_temple::peer::GeneratePredictableUuid;
using floating_temple::peer::StringToUuid;
using floating_temple::peer::Uuid;
using floating_temple::peer::UuidToString;
using google::InitGoogleLogging;
using std::printf;

namespace {

// TODO(dss): This definition is copied from peer/transaction_store.cc.
// Consolidate the two definitions.
const char kObjectNamespaceUuidString[] = "ab2d0b40fe6211e2bf8b000c2949fc67";

}  // namespace

int main(int argc, char** argv) {
  InitGoogleLogging(argv[0]);

  CHECK_EQ(argc, 2) << "Usage: generate_shared_object_id <name>";

  const Uuid object_namespace_uuid = StringToUuid(kObjectNamespaceUuidString);

  Uuid object_id;
  GeneratePredictableUuid(object_namespace_uuid, argv[1], &object_id);

  printf("%s\n", UuidToString(object_id).c_str());

  return 0;
}
