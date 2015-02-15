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

import os, subprocess

# Returns a command line string that can be used to compile a .proto file into
# .pb.h and .pb.cc files.
def get_protoc_command(source, target, env, for_signature):
  source_dir = str(env['SOURCE_DIR'])
  build_dir = str(env['BUILD_DIR'])
  proto_file = os.path.join(source_dir, str(source[0]))

  return ('%s --proto_path=%s --cpp_out=%s %s' %
          (str(env['PROTOC']), source_dir, build_dir, proto_file))

def get_aggregate_library_command(source, target, env, for_signature):
  return (('%s -o %s %s -shared -Wl,--whole-archive %s ' +
           '-Wl,--no-whole-archive %s') %
          (str(env['CXX']),
           str(target[0]),
           ' '.join([str(flag) for flag in env['LINKFLAGS']]),
           ' '.join([str(f) for f in source]),
           ' '.join(['-l' + str(lib) for lib in env['LIBS']])))

def run_command(args, cwd):
  print 'Running command %s in directory %s ...' % (repr(' '.join(args)),
                                                    repr(cwd))
  return subprocess.call(args, cwd = cwd) == 0

def configure_and_make(target, source, env):
  source_dir = os.path.dirname(Flatten(source)[0].abspath)
  build_dir = os.path.join(env['BUILD_DIR'].abspath,
                           os.path.relpath(source_dir,
                                           env['SOURCE_DIR'].abspath))

  if not os.path.exists(os.path.join(build_dir, 'Makefile')):
    # Create the build directory if it doesn't exist.
    if not os.path.exists(build_dir):
      os.makedirs(build_dir)

    # Run the configure script.
    configure_script = os.path.join(source_dir, 'configure')

    if not run_command([os.path.relpath(configure_script, build_dir)],
                       build_dir):
      return 1

  # Run make.
  if not run_command([str(env['MAKE'])], build_dir):
    return 1

  return 0

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

  # scons doesn't know how to scan .proto files for dependencies, so as a
  # workaround just make every .proto file depend on every other .proto file in
  # the same library. This doesn't affect build performance much because the
  # protocol compiler is so fast.
  env.Depends(all_cxx_files, source)

  all_cc_files = [f for f in all_cxx_files if str(f).endswith('.cc')]

  return env.Library(
      target = target,
      source = all_cc_files,
    )

def print_test_summary(msg):
  hr = '=' * len(msg)
  print hr
  print msg
  print hr

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
    print_test_summary('All %d tests passed' % (test_count,))
  else:
    print_test_summary('%d of %d tests failed' %
                       (failed_test_count, test_count))
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

disable_builtin_alloc = Split("""
    -fno-builtin-calloc -fno-builtin-free -fno-builtin-malloc
    -fno-builtin-realloc
  """)

common_flags = ['-pthread', '-fPIC']
if use_tcmalloc:
  common_flags += disable_builtin_alloc

base_env = Environment(
    SOURCE_DIR = Dir('.'),

    TAR = 'tar',
    UNZIP = 'unzip',
    MAKE = 'make',
    PROTOC = 'protoc',

    CFLAGS = common_flags,
    CXXFLAGS = common_flags + ['-fno-exceptions'],
    LINKFLAGS = common_flags,

    # TODO(dss): Use either -I or -iquote as appropriate.
    CPPPATH = Split("""
        .
        #.
        #third_party/gmock-1.7.0/gtest/include
        #third_party/gmock-1.7.0/include
      """),

    BUILDERS = {
        'Protoc' : Builder(generator = get_protoc_command),
        'AggregateLibrary' : Builder(generator = get_aggregate_library_command),
        'ConfigureAndMake' : Builder(action = configure_and_make),
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
      CFLAGS = Split('-g -O0'),
      CXXFLAGS = Split('-g -O0'),
    )
elif mode == 'rel':
  build_dir = Dir('build-rel')
  base_env.Append(
      CFLAGS = Split('-O3'),
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

SConscript('SConscript', exports = 'base_env run_tests',
           variant_dir = build_dir, duplicate = 0)