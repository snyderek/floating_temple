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

#ifndef UTIL_STL_UTIL_H_
#define UTIL_STL_UTIL_H_

#include <cstddef>

#include "base/logging.h"

namespace floating_temple {

template<class Iterator>
Iterator PrevIterator(Iterator it) {
  --it;
  return it;
}

template<class Iterator>
Iterator NextIterator(Iterator it) {
  ++it;
  return it;
}

template<class Container>
typename Container::const_iterator FindInContainer(
    const Container* haystack, const typename Container::value_type& needle) {
  CHECK(haystack != NULL);

  const typename Container::const_iterator end_it = haystack->end();
  typename Container::const_iterator it = haystack->begin();

  while (it != end_it && !(*it == needle)) {
    ++it;
  }

  return it;
}

template<class Container>
typename Container::iterator FindInContainer(
    Container* haystack, const typename Container::value_type& needle) {
  CHECK(haystack != NULL);

  const typename Container::const_iterator end_it = haystack->end();
  typename Container::iterator it = haystack->begin();

  while (it != end_it && !(*it == needle)) {
    ++it;
  }

  return it;
}

template<class Set>
Set MakeSingletonSet(const typename Set::value_type& element) {
  Set s;
  s.insert(element);
  return s;
}

}  // namespace floating_temple

#endif  // UTIL_STL_UTIL_H_
