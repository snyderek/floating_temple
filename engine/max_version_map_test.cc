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

#include "engine/max_version_map.h"

#include <string>

#include <gflags/gflags.h>

#include "base/logging.h"
#include "engine/canonical_peer.h"
#include "engine/make_transaction_id.h"
#include "engine/proto/transaction_id.pb.h"
#include "engine/transaction_id_util.h"
#include "third_party/gmock-1.7.0/gtest/include/gtest/gtest.h"

using google::InitGoogleLogging;
using google::ParseCommandLineFlags;
using testing::InitGoogleTest;

namespace floating_temple {
namespace peer {
namespace {

TEST(MaxVersionMapTest, AddPeerTransactionId) {
  MaxVersionMap version_map;

  const CanonicalPeer canonical_peer1("peer_1");
  const CanonicalPeer canonical_peer2("peer_2");
  const CanonicalPeer canonical_peer3("peer_3");
  const CanonicalPeer canonical_peer4("peer_4");

  EXPECT_TRUE(version_map.IsEmpty());

  version_map.AddPeerTransactionId(&canonical_peer1,
                                   MakeTransactionId(0x2222222222222222,
                                                     0x2222222222222222,
                                                     0x2222222222222222));
  version_map.AddPeerTransactionId(&canonical_peer2,
                                   MakeTransactionId(0x1111111111111111,
                                                     0x1111111111111111,
                                                     0x1111111111111111));
  version_map.AddPeerTransactionId(&canonical_peer3,
                                   MakeTransactionId(0x3333333333333333,
                                                     0x3333333333333333,
                                                     0x3333333333333333));

  EXPECT_FALSE(version_map.IsEmpty());

  TransactionId transaction_id;

  EXPECT_TRUE(version_map.GetPeerTransactionId(&canonical_peer1,
                                               &transaction_id));
  EXPECT_EQ("222222222222222222222222222222222222222222222222",
            TransactionIdToString(transaction_id));

  EXPECT_TRUE(version_map.GetPeerTransactionId(&canonical_peer2,
                                               &transaction_id));
  EXPECT_EQ("111111111111111111111111111111111111111111111111",
            TransactionIdToString(transaction_id));

  EXPECT_TRUE(version_map.GetPeerTransactionId(&canonical_peer3,
                                               &transaction_id));
  EXPECT_EQ("333333333333333333333333333333333333333333333333",
            TransactionIdToString(transaction_id));

  EXPECT_FALSE(version_map.GetPeerTransactionId(&canonical_peer4,
                                                &transaction_id));

  EXPECT_TRUE(version_map.HasPeerTransactionId(
                  &canonical_peer1,
                  MakeTransactionId(0x2222222222222222, 0x2222222222222222,
                                    0x2222222222222222)));
  EXPECT_FALSE(version_map.HasPeerTransactionId(
                  &canonical_peer1,
                  MakeTransactionId(0x2222222222222223, 0x2222222222222223,
                                    0x2222222222222223)));

  version_map.Clear();

  EXPECT_TRUE(version_map.IsEmpty());
}

}  // namespace
}  // namespace peer
}  // namespace floating_temple

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
