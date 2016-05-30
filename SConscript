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

# "engine" subdirectory
#
# The core of the distributed interpreter (i.e., the "engine").

engine_lib = ft_env.Library(
    target = 'engine/engine',
    source = Split("""
        engine/canonical_peer.cc
        engine/canonical_peer_map.cc
        engine/committed_event.cc
        engine/committed_value.cc
        engine/connection_manager.cc
        engine/convert_value.cc
        engine/create_network_peer.cc
        engine/deserialization_context_impl.cc
        engine/event_queue.cc
        engine/get_event_proto_type.cc
        engine/get_peer_message_type.cc
        engine/live_object_node.cc
        engine/object_reference_impl.cc
        engine/peer_connection.cc
        engine/peer_exclusion_map.cc
        engine/peer_id.cc
        engine/peer_impl.cc
        engine/pending_event.cc
        engine/playback_thread.cc
        engine/recording_thread.cc
        engine/sequence_point_impl.cc
        engine/serialization_context_impl.cc
        engine/serialize_local_object_to_string.cc
        engine/shared_object.cc
        engine/shared_object_transaction.cc
        engine/transaction_id_generator.cc
        engine/transaction_id_util.cc
        engine/transaction_sequencer.cc
        engine/transaction_store.cc
        engine/unversioned_live_object.cc
        engine/unversioned_object_content.cc
        engine/uuid_util.cc
        engine/value_proto_util.cc
        engine/versioned_live_object.cc
        engine/versioned_object_content.cc
      """),
  )

# Classes that are only used in tests.
engine_testing_lib = ft_env.Library(
    target = 'engine/engine_testing',
    source = Split("""
        engine/make_transaction_id.cc
        engine/mock_sequence_point.cc
        engine/mock_transaction_store.cc
        engine/mock_versioned_local_object.cc
      """),
  )

# "engine/proto" subdirectory
#
# Protocol message definitions for the distributed interpreter engine.

engine_proto_lib = ft_env.ProtoLibrary(
    target = 'engine/proto/engine_proto',
    source = Split("""
        engine/proto/event.proto
        engine/proto/peer.proto
        engine/proto/transaction_id.proto
        engine/proto/uuid.proto
        engine/proto/value_proto.proto
      """),
  )

# "fake_engine" subdirectory
#
# A fake engine implementation for testing. The fake engine is also used to
# implement the stand-alone version of the distributed interpreter.

fake_engine_lib = ft_env.Library(
    target = 'fake_engine/fake_engine',
    source = Split("""
        fake_engine/create_standalone_peer.cc
        fake_engine/fake_object_reference.cc
        fake_engine/fake_peer.cc
        fake_engine/fake_thread.cc
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

# "protocol_server/testdata" subdirectory
#
# Protocol messages used in tests of the protocol server.

protocol_server_test_proto_lib = ft_env.ProtoLibrary(
    target = 'protocol_server/testdata/protocol_server_test_proto',
    source = Split("""
        protocol_server/testdata/test.proto
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

# "toy_lang" subdirectory
#
# An interpreter for a toy language. This is useful for testing the engine.

toy_lang_lib = ft_env.Library(
    target = 'toy_lang/toy_lang',
    source = Split("""
        toy_lang/code_block.cc
        toy_lang/expression.cc
        toy_lang/get_serialized_expression_type.cc
        toy_lang/get_serialized_object_type.cc
        toy_lang/interpreter_impl.cc
        toy_lang/lexer.cc
        toy_lang/parser.cc
        toy_lang/program_object.cc
        toy_lang/run_toy_lang_program.cc
        toy_lang/symbol_table.cc
        toy_lang/token.cc
        toy_lang/wrap.cc
        toy_lang/zoo/add_function.cc
        toy_lang/zoo/begin_tran_function.cc
        toy_lang/zoo/bool_object.cc
        toy_lang/zoo/code_block_object.cc
        toy_lang/zoo/end_tran_function.cc
        toy_lang/zoo/for_function.cc
        toy_lang/zoo/function.cc
        toy_lang/zoo/get_variable_function.cc
        toy_lang/zoo/if_function.cc
        toy_lang/zoo/int_object.cc
        toy_lang/zoo/len_function.cc
        toy_lang/zoo/less_than_function.cc
        toy_lang/zoo/list_append_function.cc
        toy_lang/zoo/list_function.cc
        toy_lang/zoo/list_get_function.cc
        toy_lang/zoo/list_object.cc
        toy_lang/zoo/local_object_impl.cc
        toy_lang/zoo/map_get_function.cc
        toy_lang/zoo/map_is_set_function.cc
        toy_lang/zoo/map_object.cc
        toy_lang/zoo/map_set_function.cc
        toy_lang/zoo/none_object.cc
        toy_lang/zoo/not_function.cc
        toy_lang/zoo/print_function.cc
        toy_lang/zoo/range_function.cc
        toy_lang/zoo/range_iterator_object.cc
        toy_lang/zoo/set_variable_function.cc
        toy_lang/zoo/string_object.cc
        toy_lang/zoo/variable_object.cc
        toy_lang/zoo/while_function.cc
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
        engine_proto_lib,
        base_lib,
      ],
  )

ft_env.Program(
    target = 'bin/generate_named_object_id',
    source = Split("""
        bin/generate_named_object_id.cc
      """) + [
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
      ],
  )

ft_env.Program(
    target = 'bin/generate_transaction_ids',
    source = Split("""
        bin/generate_transaction_ids.cc
      """) + [
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
      ],
  )

bin_floating_toy_lang = ft_env.Program(
    target = 'bin/floating_toy_lang',
    source = Split("""
        bin/floating_toy_lang.cc
      """) + [
        # These libraries must be listed in reverse-dependency order. That is,
        # if library B depends on library A, then A must appear *after* B in the
        # list.
        toy_lang_lib,
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
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
        fake_engine_lib,
        value_lib,
        toy_lang_proto_lib,
        util_lib,
        base_lib,
      ],
  )

# Tests

base_string_printf_test = ft_env.Program(
    target = 'base/string_printf_test',
    source = Split("""
        base/string_printf_test.cc
      """) + [
        base_lib,
        gtest_lib,
      ],
  )

engine_connection_manager_test = ft_env.Program(
    target = 'engine/connection_manager_test',
    source = Split("""
        engine/connection_manager_test.cc
      """) + [
        engine_testing_lib,
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

engine_interval_set_test = ft_env.Program(
    target = 'engine/interval_set_test',
    source = Split("""
        engine/interval_set_test.cc
      """) + [
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

engine_max_version_map_test = ft_env.Program(
    target = 'engine/max_version_map_test',
    source = Split("""
        engine/max_version_map_test.cc
      """) + [
        engine_testing_lib,
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

engine_peer_id_test = ft_env.Program(
    target = 'engine/peer_id_test',
    source = Split("""
        engine/peer_id_test.cc
      """) + [
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

engine_playback_thread_test = ft_env.Program(
    target = 'engine/playback_thread_test',
    source = Split("""
        engine/playback_thread_test.cc
      """) + [
        engine_testing_lib,
        engine_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

engine_recording_thread_test = ft_env.Program(
    target = 'engine/recording_thread_test',
    source = Split("""
        engine/recording_thread_test.cc
      """) + [
        engine_testing_lib,
        engine_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

engine_toy_lang_integration_test = ft_env.Program(
    target = 'engine/toy_lang_integration_test',
    source = Split("""
        engine/toy_lang_integration_test.cc
      """) + [
        toy_lang_lib,
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        toy_lang_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

engine_transaction_store_test = ft_env.Program(
    target = 'engine/transaction_store_test',
    source = Split("""
        engine/transaction_store_test.cc
      """) + [
        engine_testing_lib,
        engine_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
        gtest_lib,
      ],
  )

engine_uuid_util_test = ft_env.Program(
    target = 'engine/uuid_util_test',
    source = Split("""
        engine/uuid_util_test.cc
      """) + [
        engine_lib,
        protocol_server_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gtest_lib,
      ],
  )

engine_shared_object_test = ft_env.Program(
    target = 'engine/shared_object_test',
    source = Split("""
        engine/shared_object_test.cc
      """) + [
        engine_testing_lib,
        engine_lib,
        protocol_server_lib,
        fake_interpreter_lib,
        value_lib,
        engine_proto_lib,
        util_lib,
        base_lib,
        gmock_lib,
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
    base_string_printf_test,
    engine_connection_manager_test,
    engine_interval_set_test,
    engine_max_version_map_test,
    engine_peer_id_test,
    engine_playback_thread_test,
    engine_recording_thread_test,
    engine_shared_object_test,
    engine_toy_lang_integration_test,
    engine_transaction_store_test,
    engine_uuid_util_test,
    protocol_server_buffer_util_test,
    protocol_server_protocol_connection_impl_test,
    protocol_server_varint_test,
    toy_lang_lexer_test,
    util_dump_context_impl_test,
    util_stl_util_test,
  ]

sh_tests = [
    File('engine/toy_lang_integration_test.sh'),
  ]

all_tests = cxx_tests + sh_tests

check_alias = ft_env.Alias('check', all_tests, run_tests)

# Add dependencies for the shell tests. This is kind of a kludge, but it works
# because I always run tests by typing "scons check".
ft_env.Depends(check_alias,
               [bin_floating_toy_lang, bin_get_unused_port_for_testing])

# Always run tests if the "check" target is specified, even if the tests haven't
# changed.
ft_env.AlwaysBuild(check_alias)
