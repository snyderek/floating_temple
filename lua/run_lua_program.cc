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

#include "lua/run_lua_program.h"

#include <cstddef>
#include <cstdio>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "include/c++/peer.h"
#include "include/c++/value.h"
#include "lua/hook_functions.h"
#include "lua/program_object.h"
#include "lua/third_party_lua_headers.h"

using std::FILE;
using std::fclose;
using std::feof;
using std::ferror;
using std::fopen;
using std::fread;
using std::size_t;
using std::string;

namespace floating_temple {

class UnversionedLocalObject;

namespace lua {
namespace {

void ReadFileContent(const string& file_name, string* content) {
  CHECK(!file_name.empty());
  CHECK(content != nullptr);

  FILE* const fp = fopen(file_name.c_str(), "r");
  PLOG_IF(FATAL, fp == nullptr) << "fopen";

  bool eof = false;
  while (!eof) {
    char buffer[1000];
    const size_t read_count = fread(buffer, sizeof(buffer[0]),
                                    ARRAYSIZE(buffer), fp);

    if (read_count < ARRAYSIZE(buffer)) {
      CHECK_EQ(ferror(fp), 0);
      CHECK_NE(feof(fp), 0);
      eof = true;
    }

    content->append(buffer, read_count);
  }

  PLOG_IF(FATAL, fclose(fp) != 0) << "fclose";
}

}  // namespace

int RunLuaProgram(Peer* peer, const string& source_file_name, bool linger) {
  CHECK(peer != nullptr);

  string file_content;
  ReadFileContent(source_file_name, &file_content);

  UnversionedLocalObject* const program_object = new ProgramObject(
      source_file_name, file_content);

  // Install the Floating Temple hooks in the Lua interpreter.
  const ft_ObjectReferencesEqualHook old_object_references_equal_hook =
      ft_installobjectreferencesequalhook(&AreObjectsEqual);
  const ft_NewTableHook old_new_table_hook = ft_installnewtablehook(
      &CreateTable);
  const ft_GetTableHook old_get_table_hook = ft_installgettablehook(
      &CallMethod_GetTable);
  const ft_SetTableHook old_set_table_hook = ft_installsettablehook(
      &CallMethod_SetTable);
  const ft_ObjLenHook old_obj_len_hook = ft_installobjlenhook(
      &CallMethod_ObjLen);
  const ft_SetListHook old_set_list_hook = ft_installsetlisthook(
      &CallMethod_SetList);
  const ft_TableInsertHook old_table_insert_hook = ft_installtableinserthook(
      &CallMethod_TableInsert);

  Value return_value;
  peer->RunProgram(program_object, "run", &return_value, linger);

  // Remove the Floating Temple hooks.
  ft_installobjectreferencesequalhook(old_object_references_equal_hook);
  ft_installnewtablehook(old_new_table_hook);
  ft_installgettablehook(old_get_table_hook);
  ft_installsettablehook(old_set_table_hook);
  ft_installobjlenhook(old_obj_len_hook);
  ft_installsetlisthook(old_set_list_hook);
  ft_installtableinserthook(old_table_insert_hook);

  return static_cast<int>(return_value.int64_value());
}

}  // namespace lua
}  // namespace floating_temple
