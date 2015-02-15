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

#ifndef PEER_INTERVAL_MAP_H_
#define PEER_INTERVAL_MAP_H_

#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "util/stl_util.h"

namespace floating_temple {
namespace peer {

template<typename T> class IntervalMap;
template<typename T> bool operator==(const IntervalMap<T>& a,
                                     const IntervalMap<T>& b);

template<typename T>
class IntervalMap {
 public:
  IntervalMap() {}
  IntervalMap(const IntervalMap<T>& other);

  // The interval contains 'start', but not 'end'.
  void AddInterval(const T& start, const T& end);

  bool Contains(const T& t) const;

  void GetEndPoints(std::vector<T>* end_points) const;

  IntervalMap<T>& operator=(const IntervalMap<T>& other);

 private:
  std::map<T, T> map_;

  template<typename S> friend bool operator==(const IntervalMap<S>& a,
                                              const IntervalMap<S>& b);
};

template<typename T>
IntervalMap<T>::IntervalMap(const IntervalMap<T>& other)
    : map_(other.map_) {
}

template<typename T>
void IntervalMap<T>::AddInterval(const T& start, const T& end) {
  if (!(start < end)) {
    return;
  }

  typename std::map<T, T>::iterator start_it = map_.upper_bound(start);

  if (start_it == map_.begin() || PrevIterator(start_it)->second < start) {
    const std::pair<typename std::map<T, T>::iterator, bool> insert_result =
        map_.insert(std::make_pair(start, end));
    CHECK(insert_result.second);
    start_it = insert_result.first;
  } else {
    --start_it;
  }

  const typename std::map<T, T>::iterator end_it = map_.upper_bound(end);
  CHECK(end_it != map_.begin());
  const T& old_end = PrevIterator(end_it)->second;

  if (start_it->second < old_end) {
    start_it->second = old_end;
  }

  ++start_it;
  map_.erase(start_it, end_it);
}

template<typename T>
bool IntervalMap<T>::Contains(const T& t) const {
  const typename std::map<T, T>::const_iterator it = map_.upper_bound(t);

  if (it == map_.begin()) {
    return false;
  }

  return t < PrevIterator(it)->second;
}

template<typename T>
void IntervalMap<T>::GetEndPoints(std::vector<T>* end_points) const {
  CHECK(end_points != NULL);

  end_points->reserve(map_.size() * 2);

  for (typename std::map<T, T>::const_iterator it = map_.begin();
       it != map_.end(); ++it) {
    end_points->push_back(it->first);
    end_points->push_back(it->second);
  }
}

template<typename T>
IntervalMap<T>& IntervalMap<T>::operator=(const IntervalMap<T>& other) {
  map_ = other.map_;
  return *this;
}

template<typename T>
bool operator==(const IntervalMap<T>& a, const IntervalMap<T>& b) {
  return a.map_ == b.map_;
}

}  // namespace peer
}  // namespace floating_temple

#endif  // PEER_INTERVAL_MAP_H_
