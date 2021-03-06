# -*- mode: python -*-
#
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

# NOTE: The purpose of this file is to parse the build options, set up the build
# environment, and create the builder objects. If you wish to add a build target
# to the project, please place it in the "SConscript" file instead.

import os
import subprocess

# Returns a command line string that can be used to compile a .proto file into
# .pb.h and .pb.cc files.
def get_protoc_command(source, target, env, for_signature):
  source_dir = str(env['SOURCE_DIR'])
  build_dir = str(env['BUILD_DIR'])
  proto_file = os.path.join(source_dir, str(source[0]))

  return subprocess.list2cmdline([
      str(env['PROTOC']),
      '--proto_path={0}'.format(source_dir),
      '--cpp_out={0}'.format(build_dir),
      proto_file,
    ])

# This is a builder method that creates a library from a list of .proto files.
def build_proto_library(env, source, target):
  all_cxx_files = []

  for f in source:
    base_name = os.path.splitext(str(f))[0]
    cxx_files = env.Protoc(
        target = [base_name + '.pb.cc', base_name + '.pb.h'],
        source = f,
      )
    all_cxx_files.extend(cxx_files)

  # SCons doesn't know how to scan .proto files for dependencies, so as a
  # workaround just make every .proto file depend on every other .proto file in
  # the same library. This doesn't affect build times much because the protocol
  # compiler is so fast.
  env.Depends(all_cxx_files, source)

  all_cc_files = [f for f in all_cxx_files if str(f).endswith('.cc')]

  return env.Library(
      target = target,
      source = all_cc_files,
    )

# Prints the specified message, with horizontal rules above and below it.
def print_test_summary(msg):
  hr = '=' * len(msg)
  print hr
  print msg
  print hr

# Runs the specified tests, one at a time. Prints a status line after each
# test's output indicating whether the test passed or failed. After all tests
# have run, prints a summary message with the number of tests that failed, and
# the names of the failing tests.
#
# 'source' is an array of test executable files. 'target' is ignored by this
# 'function. Returns 0 if all tests passed, or 1 if at least one test failed.
def run_tests(target, source, env):
  build_dir = str(env['BUILD_DIR'])

  tests = Flatten(source)
  failed_tests = []

  for f in tests:
    test_name = str(f)

    # Get the path of the test executable relative to the build directory.
    test_path = os.path.relpath(f.abspath, build_dir)

    if subprocess.call([test_path], cwd = build_dir) == 0:
      print 'PASS:', test_name
    else:
      print 'FAIL:', test_name
      failed_tests.append(test_name)

  test_count = len(tests)
  failed_test_count = len(failed_tests)
  return_code = 0

  if failed_test_count == 0:
    print_test_summary('All {0} tests passed'.format(test_count))
  else:
    print_test_summary('{0} of {1} tests failed'.format(failed_test_count,
                                                        test_count))
    print
    print 'Failed tests:'
    for f in failed_tests:
      print '   ', f
    print
    return_code = 1

  return return_code

Help("""
    Type: 'scons mode=dbg' to build the debug version.
          'scons mode=rel' to build the release version.

    Add 'use_tcmalloc=1' to build with the Thread-Caching Malloc library.
  """)

mode = ARGUMENTS.get('mode', 'dbg')
use_tcmalloc = int(ARGUMENTS.get('use_tcmalloc', 0))

common_flags = Split('-pthread')

base_env = Environment(
    SOURCE_DIR = Dir('.'),

    PROTOC = 'protoc',
    CXX = 'clang',
    LINK = 'clang',

    CXXFLAGS = common_flags + Split("""
        -fno-exceptions -std=c++11 -stdlib=libstdc++
      """),
    LINKFLAGS = common_flags,
    LIBS = Split('stdc++'),

    # TODO(dss): Use either -I or -iquote as appropriate.
    CPPPATH = Split("""
        .
        #.
        #third_party/gmock-1.7.0/gtest/include
        #third_party/gmock-1.7.0/include
      """),

    BUILDERS = {
        'Protoc' : Builder(generator = get_protoc_command),
      },
  )
base_env.AddMethod(build_proto_library, 'ProtoLibrary')

# If a source file's timestamp hasn't changed since the last build, assume that
# the file is up to date and don't bother recomputing the MD5 checksum. This
# speeds up the build process.
base_env.Decider('MD5-timestamp')

if mode == 'dbg':
  build_dir = Dir('build-dbg')
  base_env.Append(
      CXXFLAGS = Split('-g -O0'),
    )
elif mode == 'rel':
  build_dir = Dir('build-rel')
  base_env.Append(
      CXXFLAGS = Split('-O3'),
    )
else:
  print "Invalid 'mode' argument:", mode
  print "Allowed values are 'dbg' and 'rel'."
  Exit(1)

if use_tcmalloc:
  # tcmalloc, if present, must be the last library specified on the linker
  # command line.
  base_env.Append(LIBS = Split('tcmalloc'))

base_env['BUILD_DIR'] = build_dir

# All of the build targets are placed in a separate file, included here, so that
# we don't have to specify the build directory for each target. ("variant_dir"
# is what SCons calls a build directory.)
#
# "duplicate = 0" tells SCons not to copy source files to the build directory.
# All of the build actions in this project should work just fine if the source
# directory and build directory are different.
SConscript('SConscript', exports = 'base_env run_tests',
           variant_dir = build_dir, duplicate = 0)
