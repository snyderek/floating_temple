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

#ifndef UTIL_FILE_UTIL_H_
#define UTIL_FILE_UTIL_H_

#include <string>

namespace floating_temple {

std::string GetSystemTempDirName();
std::string MakeTempDir(const std::string& temp_dir_template);

std::string PathJoin(const std::string& directory, const std::string& name);

}  // namespace floating_temple

#endif  // UTIL_FILE_UTIL_H_