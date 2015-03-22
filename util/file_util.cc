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

#include "util/file_util.h"

#include <paths.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include "base/logging.h"
#include "util/string_util.h"

using std::string;
using std::unique_ptr;

namespace floating_temple {

string GetSystemTempDirName() {
  const char* tmpdir = getenv("TMPDIR");
  if (IsValidString(tmpdir)) {
    return tmpdir;
  }

#ifdef P_tmpdir
  if (IsValidString(P_tmpdir)) {
    return P_tmpdir;
  }
#endif

#ifdef _PATH_TMP
  if (IsValidString(_PATH_TMP)) {
    return _PATH_TMP;
  }
#endif

  return "/tmp";
}

string MakeTempDir(const string& temp_dir_template) {
  unique_ptr<char[]> buffer(CreateCharBuffer(temp_dir_template));

  if (mkdtemp(buffer.get()) == nullptr) {
    PLOG(FATAL) << "mkdtemp";
  }

  return buffer.get();
}

string PathJoin(const string& directory, const string& name) {
  CHECK(!name.empty());
  CHECK_NE(name[0], '/');

  string result = directory;
  const string::size_type directory_length = directory.length();
  if (directory_length == 0 || directory[directory_length - 1] != '/') {
    result += '/';
  }
  result += name;

  return result;
}

}  // namespace floating_temple
