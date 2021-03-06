#!/bin/bash

# Floating Temple
# Copyright 2015 Derek S. Snyder
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -o errexit
set -o nounset
set -o xtrace

# TODO(dss): The path to the fibonacci.toy file below shouldn't depend on the
# location of the build directory relative to the source directory.

./bin/floating_toy_lang \
  --peer_port="$(./bin/get_unused_port_for_testing)" --linger=false \
  ../toy_lang/testdata/fibonacci.toy
