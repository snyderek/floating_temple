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

#ifndef BASE_STRING_PRINTF_H_
#define BASE_STRING_PRINTF_H_

#include <cstdarg>
#include <string>

#ifdef __GNUC__
#define FORMAT_ATTRIBUTE(archetype, string_index, first_to_check) \
  __attribute__((__format__(archetype, string_index, first_to_check)))
#else
#define FORMAT_ATTRIBUTE(archetype, string_index, first_to_check)
#endif

namespace floating_temple {

// These functions work like the sprintf family of functions, except that they
// store the result in a string object instead of in a buffer allocated by the
// caller.
//
// All of the pointer parameters below must be non-NULL.

// Returns the result.
std::string StringPrintf(const char* format, ...)
    FORMAT_ATTRIBUTE(__gnu_printf__, 1, 2);
// Stores the result in *str.
void SStringPrintf(std::string* str, const char* format, ...)
    FORMAT_ATTRIBUTE(__gnu_printf__, 2, 3);
// Appends the result to *str.
void StringAppendF(std::string* str, const char* format, ...)
    FORMAT_ATTRIBUTE(__gnu_printf__, 2, 3);

// Same as the above, except that these functions accept a va_list.
std::string StringPrintfV(const char* format, std::va_list arg_list)
    FORMAT_ATTRIBUTE(__gnu_printf__, 1, 0);
void SStringPrintfV(std::string* str, const char* format, std::va_list arg_list)
    FORMAT_ATTRIBUTE(__gnu_printf__, 2, 0);
void StringAppendFV(std::string* str, const char* format, std::va_list arg_list)
    FORMAT_ATTRIBUTE(__gnu_printf__, 2, 0);

}  // namespace floating_temple

#undef FORMAT_ATTRIBUTE

#endif  // BASE_STRING_PRINTF_H_
