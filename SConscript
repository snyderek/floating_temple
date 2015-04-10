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

Import('base_env run_tests')

common_warnings = Split("""
    -Wall -Wextra -Werror -Wcast-align -Wcast-qual -Wdisabled-optimization
    -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations
    -Wmissing-format-attribute -Wno-missing-field-initializers
    -Wno-unused-parameter -Wredundant-decls -Wstrict-overflow=5 -Wuninitialized
  """)
c_warnings = Split("""
  """)
cxx_warnings = Split("""
    -Woverloaded-virtual -Wstrict-null-sentinel
  """)

ft_env = base_env.Clone()
ft_env.Append(
    CPPFLAGS = Split("""
        -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
      """),
    CFLAGS = common_warnings + c_warnings,
    CXXFLAGS = common_warnings + cxx_warnings,
  )
ft_env.Prepend(
    LIBS = Split('gflags glog protobuf rt uuid'),
  )

# "base" subdirectory
#
# Non-project-specific support code. None of the code in "base" should depend on
# anything that's not in "base".

base_lib = ft_env.Library(
    target = 'base/base',
    source = Split("""
        base/cond_var.cc
        base/escape.cc
        base/hex.cc
        base/logging.cc
        base/mutex.cc
        base/mutex_lock.cc
        base/notification.cc
        base/random.cc
        base/string_printf.cc
        base/thread_safe_counter.cc
        base/time_util.cc
      """),
  )

# "c_harness" subdirectory

c_harness_lib = ft_env.Library(
    target = 'c_harness/c_harness',
    source = Split("""
        c_harness/logging.cc
        c_harness/peer.cc
        c_harness/proxy_interpreter.cc
        c_harness/proxy_local_object.cc
        c_harness/value.cc
      """),
  )

# "fake_interpreter" subdirectory
#
# A fake local interpreter implementation for testing.

fake_interpreter_lib = ft_env.Library(
    target = 'fake_interpreter/fake_interpreter',
    source = Split("""
        fake_interpreter/fake_interpreter.cc
        fake_interpreter/fake_local_object.cc
      """),
  )

# "fake_peer" subdirectory
#
# A fake peer implementation for testing.

fake_peer_lib = ft_env.Library(
    target = 'fake_peer/fake_peer',
    source = Split("""
        fake_peer/create_standalone_peer.cc
        fake_peer/fake_peer.cc
        fake_peer/fake_peer_object.cc
        fake_peer/fake_thread.cc
      """),
  )

# "lua" subdirectory
#
# Implementation of the local interpreter for the Lua language. Uses the
# third-party Lua interpreter.

lua_env = ft_env.Clone()
lua_env.Append(
    CFLAGS = Split('-DLUA_COMPAT_ALL -DLUA_USE_LINUX'),

    CPPPATH = Split("""
        third_party/lua-5.2.3/src
      """),
  )

lua_lib = lua_env.Library(
    target = 'lua/lua',
    source = Split("""
        lua/convert_value.cc
        lua/interpreter_impl.cc
        lua/thread_substitution.cc
      """),
  )

# "peer" subdirectory
#
# The core of the distributed interpreter (i.e., the "peer").

peer_lib = ft_env.Library(
    target = 'peer/peer',
    source = Split("""
        peer/canonical_peer.cc
        peer/canonical_peer_map.cc
        peer/committed_event.cc
        peer/committed_value.cc
        peer/connection_manager.cc
        peer/const_live_object_ptr.cc
        peer/convert_value.cc
        peer/create_network_peer.cc
        peer/deserialization_context_impl.cc
        peer/event_queue.cc
        peer/get_event_proto_type.cc
        peer/get_peer_message_type.cc
        peer/interpreter_thread.cc
        peer/live_object.cc
        peer/live_object_node.cc
        peer/live_object_ptr.cc
        peer/max_version_map.cc
        peer/min_version_map.cc
        peer/peer_connection.cc
        peer/peer_exclusion_map.cc
        peer/peer_id.cc
        peer/peer_impl.cc
        peer/peer_object_impl.cc
        peer/peer_thread.cc
        peer/pending_event.cc
        peer/sequence_point_impl.cc
        peer/serialization_context_impl.cc
        peer/serialize_local_object_to_string.cc
        peer/shared_object.cc
        peer/shared_object_transaction.cc
        peer/transaction_id_generator.cc
        peer/transaction_id_util.cc
        peer/transaction_sequencer.cc
        peer/transaction_store.cc
        peer/uuid_util.cc
        peer/value_proto_util.cc
      """),
  )

peer_testing_lib = ft_env.Library(
    target = 'peer/peer_testing',
    source = Split("""
        peer/make_transaction_id.cc
        peer/mock_local_object.cc
        peer/mock_sequence_point.cc
        peer/mock_transaction_store.cc
      """),
  )

# "peer/proto" subdirectory
#
# Protocol message definitions for the peer.

peer_proto_lib = ft_env.ProtoLibrary(
    target = 'peer/proto/peer_proto',
    source = Split("""
        peer/proto/event.proto
        peer/proto/peer.proto
        peer/proto/transaction_id.proto
        peer/proto/uuid.proto
        peer/proto/value_proto.proto
      """),
  )

# "protocol_server" subdirectory
#
# Code for sending and receiving protocol messages over socket connections.

protocol_server_lib = ft_env.Library(
    target = 'protocol_server/protocol_server',
    source = Split("""
        protocol_server/buffer_util.cc
        protocol_server/parse_protocol_message.cc
        protocol_server/protocol_server.cc
        protocol_server/varint.cc
      """),
  )

# "proto/testdata" subdirectory

protocol_server_test_proto_lib = ft_env.ProtoLibrary(
    target = 'protocol_server/testdata/protocol_server_test_proto',
    source = Split("""
        protocol_server/testdata/test.proto
      """),
  )

# "python" subdirectory
#
# Implementation of the local interpreter for the Python language. Uses the
# third-party Python interpreter.

python_env = ft_env.Clone()
python_env.Append(
    CPPPATH = Split("""
        third_party/Python-3.4.2
        third_party/Python-3.4.2/Include
      """),

    LIBS = Split('dl util'),
  )

python_lib = python_env.Library(
    target = 'python/python',
    source = Split("""
        python/call_method.cc
        python/dict_local_object.cc
        python/false_local_object.cc
        python/get_serialized_object_type.cc
        python/interpreter_impl.cc
        python/list_local_object.cc
        python/local_object_impl.cc
        python/long_local_object.cc
        python/make_value.cc
        python/method_context.cc
        python/none_local_object.cc
        python/peer_module.cc
        python/program_object.cc
        python/py_proxy_object.cc
        python/python_gil_lock.cc
        python/python_scoped_ptr.cc
        python/run_python_program.cc
        python/thread_substitution.cc
        python/true_local_object.cc
        python/unserializable_local_object.cc
      """),
  )

# "python/proto" subdirectory
#
# Serialization protocol for Python objects.

python_proto_lib = ft_env.ProtoLibrary(
    target = 'python/proto/python_proto',
    source = Split("""
        python/proto/local_type.proto
        python/proto/serialization.proto
      """),
  )

# "third_party" subdirectory
#
# Code written by other people.

gmock_env = base_env.Clone(
    # TODO(dss): Use either -iquote or -isystem as appropriate.
    CPPPATH = Split("""
        third_party/gmock-1.7.0/gtest/include
        third_party/gmock-1.7.0/include
        third_party/gmock-1.7.0/gtest
        third_party/gmock-1.7.0
      """),
  )

gmock_lib = gmock_env.Library(
    target = 'third_party/gmock',
    source = 'third_party/gmock-1.7.0/src/gmock-all.cc',
  )

gtest_lib = gmock_env.Library(
    target = 'third_party/gtest',
    source = 'third_party/gmock-1.7.0/gtest/src/gtest-all.cc',
  )

third_party_lua_env = Environment(
    CFLAGS = Split('-O2 -Wall -DLUA_COMPAT_ALL -DLUA_USE_LINUX'),
    LINKFLAGS = Split('-Wl,-E'),

    CPPPATH = Split("""
        third_party/lua-5.2.3/src
      """),

    LIBS = Split('dl m readline'),
  )

third_party_lua_lib = third_party_lua_env.Library(
    target = 'third_party/lua-5.2.3/lua',
    source = Split("""
        third_party/lua-5.2.3/src/floating_temple.c
        third_party/lua-5.2.3/src/lapi.c
        third_party/lua-5.2.3/src/lauxlib.c
        third_party/lua-5.2.3/src/lbaselib.c
        third_party/lua-5.2.3/src/lbitlib.c
        third_party/lua-5.2.3/src/lcode.c
        third_party/lua-5.2.3/src/lcorolib.c
        third_party/lua-5.2.3/src/lctype.c
        third_party/lua-5.2.3/src/ldblib.c
        third_party/lua-5.2.3/src/ldebug.c
        third_party/lua-5.2.3/src/ldo.c
        third_party/lua-5.2.3/src/ldump.c
        third_party/lua-5.2.3/src/lfunc.c
        third_party/lua-5.2.3/src/lgc.c
        third_party/lua-5.2.3/src/linit.c
        third_party/lua-5.2.3/src/liolib.c
        third_party/lua-5.2.3/src/llex.c
        third_party/lua-5.2.3/src/lmathlib.c
        third_party/lua-5.2.3/src/lmem.c
        third_party/lua-5.2.3/src/loadlib.c
        third_party/lua-5.2.3/src/lobject.c
        third_party/lua-5.2.3/src/lopcodes.c
        third_party/lua-5.2.3/src/loslib.c
        third_party/lua-5.2.3/src/lparser.c
        third_party/lua-5.2.3/src/lstate.c
        third_party/lua-5.2.3/src/lstring.c
        third_party/lua-5.2.3/src/lstrlib.c
        third_party/lua-5.2.3/src/ltable.c
        third_party/lua-5.2.3/src/ltablib.c
        third_party/lua-5.2.3/src/ltm.c
        third_party/lua-5.2.3/src/lundump.c
        third_party/lua-5.2.3/src/lvm.c
        third_party/lua-5.2.3/src/lzio.c
      """),
  )

lua_env.Depends(lua_lib, third_party_lua_lib)

third_party_lua_env.Program(
    target = 'third_party/lua-5.2.3/lua',
    source = Split("""
        third_party/lua-5.2.3/src/lua.c
      """) + [
        third_party_lua_lib,
      ],
  )

third_party_lua_env.Program(
    target = 'third_party/lua-5.2.3/luac',
    source = Split("""
        third_party/lua-5.2.3/src/luac.c
      """) + [
        third_party_lua_lib,
      ],
  )

# TODO(dss): Call 'make' on the third-party Python sub-project if any of its
# source files change. Currently, the sub-project will only be remade if the
# targets listed below are missing.
third_party_python_lib = base_env.ConfigureAndMake(
    target = Split("""
        third_party/Python-3.4.2/libpython3.4m.a
        third_party/Python-3.4.2/pyconfig.h
      """),
    source = 'third_party/Python-3.4.2/configure',
  )

python_env.Depends(python_lib, third_party_python_lib)

# "toy_lang" subdirectory
#
# An interpreter for a toy language. This is useful for testing the peer.

toy_lang_lib = ft_env.Library(
    target = 'toy_lang/toy_lang',
    source = Split("""
        toy_lang/expression.cc
        toy_lang/get_serialized_expression_type.cc
        toy_lang/get_serialized_object_type.cc
        toy_lang/interpreter_impl.cc
        toy_lang/lexer.cc
        toy_lang/local_object_impl.cc
        toy_lang/parser.cc
        toy_lang/program_object.cc
        toy_lang/run_toy_lang_program.cc
        toy_lang/symbol_table.cc
        toy_lang/token.cc
      """),
  )

# "toy_lang/proto" subdirectory
#
# Protocol message definitions that are only used by the toy language local
# interpreter.

toy_lang_proto_lib = ft_env.ProtoLibrary(
    target = 'toy_lang/proto/toy_lang_proto',
    source = Split("""
        toy_lang/proto/serialization.proto
      """),
  )

# "util" subdirectory
#
# Utility code that's commonly used throughout the Floating Temple project. Code
# in "util" may depend on code in "base" or "util", but nothing else.

util_lib = ft_env.Library(
    target = 'util/util',
    source = Split("""
        util/bool_variable.cc
        util/comma_separated.cc
        util/dump_context_impl.cc
        util/event_fd.cc
        util/file_util.cc
        util/math_util.cc
        util/signal_handler.cc
        util/socket_util.cc
        util/state_variable.cc
        util/string_util.cc
        util/tcp.cc
      """),
  )

# "value" subdirectory
#
# Implementation of the Value class, declared in "include/c++/value.h".

value_lib = ft_env.Library(
    target = 'value/value',
    source = Split("""
        value/value.cc
      """),
  )

# Shared library

# TODO(dss): Only export symbols from the shared library that are part of the
# public interface.
ft_env.AggregateLibrary(
    target = 'lib/libfloating_temple.so',
    source = [
        base_lib,
        c_harness_lib,
        fake_peer_lib,
        peer_lib,
        peer_proto_lib,
        protocol_server_lib,
        value_lib,
      ],
  )

# Executables

# TODO(dss): Put these targets in alphabetical order.

bin_get_unused_port_for_testing = ft_env.Program(
    target = 'bin/get_unused_port_for_testing',
    source = Split("""
        bin/get_unused_port_for_testing.cc
      """) + [
        util_lib,
        base_lib,
      ],
  )

ft_env.Program(
    target = 'bin/encode_proto_buf',
    source = Split("""
        bin/encode_proto_buf.cc
      """) + [
        peer_proto_lib,
        base_lib,
      ],
  )

ft_env.Program(
    target = 'bin/generate_named_object_id',
    source = Split("""
        bin/generate_named_object_id.cc
      """) + [
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
      ],
  )

ft_env.Program(
    target = 'bin/generate_transaction_ids',
    source = Split("""
        bin/generate_transaction_ids.cc
      """) + [
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
      ],
  )

bin_floating_python = python_env.Program(
    target = 'bin/floating_python',
    source = Split("""
        bin/floating_python.cc
      """) + [
        # These libraries must be listed in reverse-dependency order. That is,
        # if library B depends on library A, then A must appear _after_ B in the
        # list.
        python_lib,
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        python_proto_lib,
        util_lib,
        base_lib,
        third_party_python_lib,
      ],
  )

bin_floating_toy_lang = ft_env.Program(
    target = 'bin/floating_toy_lang',
    source = Split("""
        bin/floating_toy_lang.cc
      """) + [
        # These libraries must be listed in reverse-dependency order. That is,
        # if library B depends on library A, then A must appear _after_ B in the
        # list.
        toy_lang_lib,
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        toy_lang_proto_lib,
        util_lib,
        base_lib,
      ],
  )

ft_env.Program(
    target = 'bin/toy_lang',
    source = Split("""
        bin/toy_lang.cc
      """) + [
        toy_lang_lib,
        fake_peer_lib,
        value_lib,
        toy_lang_proto_lib,
        base_lib,
      ],
  )

# Tests

base_const_shared_ptr_test = ft_env.Program(
    target = 'base/const_shared_ptr_test',
    source = Split("""
        base/const_shared_ptr_test.cc
      """) + [
        base_lib,
        gtest_lib,
      ],
  )

base_string_printf_test = ft_env.Program(
    target = 'base/string_printf_test',
    source = Split("""
        base/string_printf_test.cc
      """) + [
        base_lib,
        gtest_lib,
      ],
  )

peer_connection_manager_test = ft_env.Program(
    target = 'peer/connection_manager_test',
    source = Split("""
        peer/connection_manager_test.cc
      """) + [
        peer_testing_lib,
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

peer_interpreter_thread_test = ft_env.Program(
    target = 'peer/interpreter_thread_test',
    source = Split("""
        peer/interpreter_thread_test.cc
      """) + [
        peer_testing_lib,
        peer_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

peer_interval_set_test = ft_env.Program(
    target = 'peer/interval_set_test',
    source = Split("""
        peer/interval_set_test.cc
      """) + [
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

peer_max_version_map_test = ft_env.Program(
    target = 'peer/max_version_map_test',
    source = Split("""
        peer/max_version_map_test.cc
      """) + [
        peer_testing_lib,
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

peer_peer_id_test = ft_env.Program(
    target = 'peer/peer_id_test',
    source = Split("""
        peer/peer_id_test.cc
      """) + [
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

peer_peer_thread_test = ft_env.Program(
    target = 'peer/peer_thread_test',
    source = Split("""
        peer/peer_thread_test.cc
      """) + [
        peer_testing_lib,
        peer_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

peer_shared_object_test = ft_env.Program(
    target = 'peer/shared_object_test',
    source = Split("""
        peer/shared_object_test.cc
      """) + [
        peer_testing_lib,
        peer_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

peer_toy_lang_integration_test = ft_env.Program(
    target = 'peer/toy_lang_integration_test',
    source = Split("""
        peer/toy_lang_integration_test.cc
      """) + [
        toy_lang_lib,
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        toy_lang_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

peer_transaction_store_test = ft_env.Program(
    target = 'peer/transaction_store_test',
    source = Split("""
        peer/transaction_store_test.cc
      """) + [
        peer_testing_lib,
        peer_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

peer_uuid_util_test = ft_env.Program(
    target = 'peer/uuid_util_test',
    source = Split("""
        peer/uuid_util_test.cc
      """) + [
        peer_lib,
        protocol_server_lib,
        value_lib,
        peer_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

protocol_server_buffer_util_test = ft_env.Program(
    target = 'protocol_server/buffer_util_test',
    source = Split("""
        protocol_server/buffer_util_test.cc
      """) + [
        protocol_server_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

protocol_server_protocol_connection_impl_test = ft_env.Program(
    target = 'protocol_server/protocol_connection_impl_test',
    source = Split("""
        protocol_server/protocol_connection_impl_test.cc
      """) + [
        protocol_server_lib,
        util_lib,
        base_lib,
        protocol_server_test_proto_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

protocol_server_varint_test = ft_env.Program(
    target = 'protocol_server/varint_test',
    source = Split("""
        protocol_server/varint_test.cc
      """) + [
        protocol_server_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

python_interpreter_impl_test = python_env.Program(
    target = 'python/interpreter_impl_test',
    source = Split("""
        python/interpreter_impl_test.cc
      """) + [
        python_lib,
        fake_peer_lib,
        value_lib,
        python_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
        third_party_python_lib,
      ],
  )

toy_lang_lexer_test = ft_env.Program(
    target = 'toy_lang/lexer_test',
    source = Split("""
        toy_lang/lexer_test.cc
      """) + [
        toy_lang_lib,
        value_lib,
        toy_lang_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

toy_lang_local_object_impl_test = ft_env.Program(
    target = 'toy_lang/local_object_impl_test',
    source = Split("""
        toy_lang/local_object_impl_test.cc
      """) + [
        toy_lang_lib,
        value_lib,
        toy_lang_proto_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

util_dump_context_impl_test = ft_env.Program(
    target = 'util/dump_context_impl_test',
    source = Split("""
        util/dump_context_impl_test.cc
      """) + [
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

util_stl_util_test = ft_env.Program(
    target = 'util/stl_util_test',
    source = Split("""
        util/stl_util_test.cc
      """) + [
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

cxx_tests = [
    base_const_shared_ptr_test,
    base_string_printf_test,
    peer_connection_manager_test,
    peer_interpreter_thread_test,
    peer_interval_set_test,
    peer_max_version_map_test,
    peer_peer_id_test,
    peer_peer_thread_test,
    peer_shared_object_test,
    peer_toy_lang_integration_test,
    peer_transaction_store_test,
    peer_uuid_util_test,
    protocol_server_buffer_util_test,
    protocol_server_protocol_connection_impl_test,
    protocol_server_varint_test,
    python_interpreter_impl_test,
    toy_lang_lexer_test,
    toy_lang_local_object_impl_test,
    util_dump_context_impl_test,
    util_stl_util_test,
  ]

sh_tests = [
    File('peer/toy_lang_integration_test.sh'),
    File('python/fib_list_test.sh'),
    File('python/floating_python_test.sh'),
  ]

all_tests = cxx_tests + sh_tests

check_alias = ft_env.Alias('check', all_tests, run_tests)
ft_env.Depends(check_alias,
               [bin_floating_python, bin_floating_toy_lang,
                bin_get_unused_port_for_testing])
ft_env.AlwaysBuild(check_alias)
