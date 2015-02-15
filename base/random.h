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

#ifndef BASE_RANDOM_H_
#define BASE_RANDOM_H_

namespace floating_temple {

// Returns a pseudo-random integer in the range 0 to RAND_MAX, inclusive.
// RAND_MAX is defined in the standard C library header stdlib.h.
//
// This function is thread-safe. When it's called for the first time on a given
// thread, the random number generator is seeded with the current time.
int GetRandomInt();

}  // namespace floating_temple

#endif  // BASE_RANDOM_H_
