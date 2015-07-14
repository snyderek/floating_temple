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

#include "engine/uuid_util.h"

#include <endian.h>

#include <cinttypes>
#include <cstddef>
#include <string>

#include <uuid.h>

#include "base/hex.h"
#include "base/integral_types.h"
#include "base/logging.h"
#include "base/string_printf.h"
#include "engine/proto/uuid.pb.h"

using std::size_t;
using std::string;

namespace floating_temple {
namespace peer {
namespace {

uint64 ParseHexUint64(const string& hex) {
  // Expected length of the input string.
  static const int kLength = 16;

  CHECK_EQ(hex.length(), static_cast<string::size_type>(kLength));

  uint64 n = 0;

  for (int i = 0; i < kLength; ++i) {
    const int hex_digit = ParseHexDigit(hex[i]);
    CHECK_GE(hex_digit, 0);

    n = n * 16u + static_cast<uint64>(hex_digit);
  }

  return n;
}

void BinaryToUuid(const unsigned char* buffer, size_t buffer_size, Uuid* uuid) {
  CHECK(buffer != nullptr);
  CHECK_EQ(buffer_size, 16u);
  CHECK(uuid != nullptr);

  const uint64* const words = reinterpret_cast<const uint64*>(buffer);

  uuid->Clear();
  uuid->set_high_word(be64toh(words[0]));
  uuid->set_low_word(be64toh(words[1]));
}

void UuidToBinary(const Uuid& uuid, unsigned char* buffer, size_t buffer_size) {
  uuid.CheckInitialized();
  CHECK(buffer != nullptr);
  CHECK_EQ(buffer_size, 16u);

  uint64* const words = reinterpret_cast<uint64*>(buffer);
  words[0] = htobe64(uuid.high_word());
  words[1] = htobe64(uuid.low_word());
}

}  // namespace

void GenerateUuid(Uuid* uuid) {
  uuid_t* uuid_context = nullptr;
  CHECK_EQ(uuid_create(&uuid_context), UUID_RC_OK);
  CHECK_EQ(uuid_make(uuid_context, UUID_MAKE_V1), UUID_RC_OK);

  unsigned char buffer[UUID_LEN_BIN];
  unsigned char* data_ptr = buffer;
  size_t data_len = sizeof buffer;
  CHECK_EQ(uuid_export(uuid_context, UUID_FMT_BIN,
                       static_cast<void*>(&data_ptr), &data_len), UUID_RC_OK);

  CHECK_EQ(uuid_destroy(uuid_context), UUID_RC_OK);

  BinaryToUuid(buffer, sizeof buffer, uuid);
}

void GeneratePredictableUuid(const Uuid& ns_uuid, const string& name,
                             Uuid* uuid) {
  string ns_uuid_str;
  UuidToHyphenatedString(ns_uuid, &ns_uuid_str);

  uuid_t* uuid_context = nullptr;
  CHECK_EQ(uuid_create(&uuid_context), UUID_RC_OK);
  CHECK_EQ(uuid_make(uuid_context, UUID_MAKE_V5, ns_uuid_str.c_str(),
                     name.c_str()), UUID_RC_OK);

  unsigned char buffer[UUID_LEN_BIN];
  unsigned char* data_ptr = buffer;
  size_t data_len = sizeof buffer;
  CHECK_EQ(uuid_export(uuid_context, UUID_FMT_BIN,
                       static_cast<void*>(&data_ptr), &data_len), UUID_RC_OK);

  CHECK_EQ(uuid_destroy(uuid_context), UUID_RC_OK);

  BinaryToUuid(buffer, sizeof buffer, uuid);
}

int CompareUuids(const Uuid& a, const Uuid& b) {
  const uint64 a_high = a.high_word();
  const uint64 b_high = b.high_word();

  if (a_high != b_high) {
    return a_high < b_high ? -1 : 1;
  }

  const uint64 a_low = a.low_word();
  const uint64 b_low = b.low_word();

  if (a_low != b_low) {
    return a_low < b_low ? -1 : 1;
  }

  return 0;
}

string UuidToString(const Uuid& uuid) {
  return StringPrintf("%016" PRIx64 "%016" PRIx64, uuid.high_word(),
                      uuid.low_word());
}

Uuid StringToUuid(const string& s) {
  CHECK_EQ(s.length(), 32u);

  Uuid uuid;
  uuid.set_high_word(ParseHexUint64(s.substr(0, 16u)));
  uuid.set_low_word(ParseHexUint64(s.substr(16u)));

  return uuid;
}

void UuidToHyphenatedString(const Uuid& uuid, string* s) {
  CHECK(s != nullptr);

  unsigned char buffer[UUID_LEN_BIN];
  UuidToBinary(uuid, buffer, sizeof buffer);

  uuid_t* uuid_context = nullptr;
  CHECK_EQ(uuid_create(&uuid_context), UUID_RC_OK);
  CHECK_EQ(uuid_import(uuid_context, UUID_FMT_BIN, buffer, sizeof buffer),
           UUID_RC_OK);

  char uuid_str[UUID_LEN_STR + 1];
  char* data_ptr = uuid_str;
  size_t data_len = sizeof uuid_str;
  CHECK_EQ(uuid_export(uuid_context, UUID_FMT_STR,
                       static_cast<void*>(&data_ptr), &data_len), UUID_RC_OK);

  CHECK_EQ(uuid_destroy(uuid_context), UUID_RC_OK);

  s->assign(data_ptr, data_len - 1);
}

}  // namespace peer
}  // namespace floating_temple

namespace std {

bool operator<(const floating_temple::peer::Uuid& a,
               const floating_temple::peer::Uuid& b) {
  return floating_temple::peer::CompareUuids(a, b) < 0;
}

}  // namespace std
