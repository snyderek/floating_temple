import sys

from pypy.interpreter.mixedmodule import MixedModule
from pypy.module.imp.importing import get_pyc_magic

class BuildersModule(MixedModule):
    appleveldefs = {}

    interpleveldefs = {
        "StringBuilder": "interp_builders.W_StringBuilder",
        "UnicodeBuilder": "interp_builders.W_UnicodeBuilder",
    }

class TimeModule(MixedModule):
    appleveldefs = {}
    interpleveldefs = {}
    if sys.platform.startswith("linux") or 'bsd' in sys.platform:
        from pypy.module.__pypy__ import interp_time
        interpleveldefs["clock_gettime"] = "interp_time.clock_gettime"
        interpleveldefs["clock_getres"] = "interp_time.clock_getres"
        for name in [
            "CLOCK_REALTIME", "CLOCK_MONOTONIC", "CLOCK_MONOTONIC_RAW",
            "CLOCK_PROCESS_CPUTIME_ID", "CLOCK_THREAD_CPUTIME_ID"
        ]:
            if getattr(interp_time, name) is not None:
                interpleveldefs[name] = "space.wrap(interp_time.%s)" % name


class ThreadModule(MixedModule):
    appleveldefs = {
        'signals_enabled': 'app_signal.signals_enabled',
    }
    interpleveldefs = {
        '_signals_enter':  'interp_signal.signals_enter',
        '_signals_exit':   'interp_signal.signals_exit',
    }


class IntOpModule(MixedModule):
    appleveldefs = {}
    interpleveldefs = {
        'int_add':         'interp_intop.int_add',
        'int_sub':         'interp_intop.int_sub',
        'int_mul':         'interp_intop.int_mul',
        'int_floordiv':    'interp_intop.int_floordiv',
        'int_mod':         'interp_intop.int_mod',
        'int_lshift':      'interp_intop.int_lshift',
        'int_rshift':      'interp_intop.int_rshift',
        'uint_rshift':     'interp_intop.uint_rshift',
    }


class OsModule(MixedModule):
    appleveldefs = {}
    interpleveldefs = {
        'real_getenv': 'interp_os.real_getenv'
    }


class Module(MixedModule):
    appleveldefs = {
    }

    interpleveldefs = {
        'internal_repr'             : 'interp_magic.internal_repr',
        'bytebuffer'                : 'bytebuffer.bytebuffer',
        'identity_dict'             : 'interp_identitydict.W_IdentityDict',
        'debug_start'               : 'interp_debug.debug_start',
        'debug_print'               : 'interp_debug.debug_print',
        'debug_stop'                : 'interp_debug.debug_stop',
        'debug_print_once'          : 'interp_debug.debug_print_once',
        'debug_flush'               : 'interp_debug.debug_flush',
        'builtinify'                : 'interp_magic.builtinify',
        'lookup_special'            : 'interp_magic.lookup_special',
        'do_what_I_mean'            : 'interp_magic.do_what_I_mean',
        'validate_fd'               : 'interp_magic.validate_fd',
        'resizelist_hint'           : 'interp_magic.resizelist_hint',
        'newlist_hint'              : 'interp_magic.newlist_hint',
        'add_memory_pressure'       : 'interp_magic.add_memory_pressure',
        'newdict'                   : 'interp_dict.newdict',
        'strategy'                  : 'interp_magic.strategy',  # dict,set,list
        'set_debug'                 : 'interp_magic.set_debug',
        'locals_to_fast'            : 'interp_magic.locals_to_fast',
    }
    if sys.platform == 'win32':
        interpleveldefs['get_console_cp'] = 'interp_magic.get_console_cp'

    submodules = {
        "builders": BuildersModule,
        "time": TimeModule,
        "thread": ThreadModule,
        "intop": IntOpModule,
        "os": OsModule,
    }

    def setup_after_space_initialization(self):
        """NOT_RPYTHON"""
        if not self.space.config.translating:
            self.extra_interpdef('interp_pdb', 'interp_magic.interp_pdb')
        if self.space.config.objspace.std.withmethodcachecounter:
            self.extra_interpdef('method_cache_counter',
                                 'interp_magic.method_cache_counter')
            self.extra_interpdef('reset_method_cache_counter',
                                 'interp_magic.reset_method_cache_counter')
            if self.space.config.objspace.std.withmapdict:
                self.extra_interpdef('mapdict_cache_counter',
                                     'interp_magic.mapdict_cache_counter')
        PYC_MAGIC = get_pyc_magic(self.space)
        self.extra_interpdef('PYC_MAGIC', 'space.wrap(%d)' % PYC_MAGIC)
        #
        try:
            from rpython.jit.backend import detect_cpu
            model = detect_cpu.autodetect()
            self.extra_interpdef('cpumodel', 'space.wrap(%r)' % model)
        except Exception:
            if self.space.config.translation.jit:
                raise
            else:
                pass   # ok fine to ignore in this case
        #
        if self.space.config.translation.jit:
            features = detect_cpu.getcpufeatures(model)
            self.extra_interpdef('jit_backend_features',
                                    'space.wrap(%r)' % features)
