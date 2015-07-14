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

#include <gflags/gflags.h>

#include "base/logging.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/transaction_id_generator.h"
#include "engine/transaction_id_util.h"

using floating_temple::peer::TransactionId;
using floating_temple::peer::TransactionIdGenerator;
using floating_temple::peer::TransactionIdToString;
using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using std::printf;

DEFINE_int32(count, 1, "Number of transaction IDs to generate");

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);

  TransactionIdGenerator generator;

  for (int i = 0; i < FLAGS_count; ++i) {
    TransactionId transaction_id;
    generator.Generate(&transaction_id);

    printf("%s\n", TransactionIdToString(transaction_id).c_str());
  }

  return 0;
}
