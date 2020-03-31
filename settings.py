import sys
import os
import platform

V8_GIT_TAG_STABLE = "7.9.317.33"
V8_GIT_TAG_MASTER = "master"

STPYV8_HOME = os.path.dirname(os.path.realpath(__file__))
DEPOT_HOME = os.environ.get('DEPOT_HOME', os.path.join(STPYV8_HOME, 'depot_tools'))
V8_HOME = os.environ.get('V8_HOME', os.path.join(STPYV8_HOME, 'v8'))

if not os.path.basename(os.path.normpath(V8_HOME)) == "v8":
    raise Exception("V8_HOME must have last path component 'v8'")

STPYV8_DEBUG = os.environ.get('STPYV8_DEBUG', False)
STPYV8_V8_GIT_TAG = os.environ.get('STPYV8_V8_GIT_TAG', V8_GIT_TAG_STABLE)
STPYV8_SKIP_DEPOT = os.environ.get('STPYV8_SKIP_DEPOT', False)
STPYV8_SKIP_V8_CHECKOUT = os.environ.get('STPYV8_SKIP_V8_CHECKOUT', False)

V8_GIT_URL = "https://chromium.googlesource.com/v8/v8.git"
DEPOT_GIT_URL = "https://chromium.googlesource.com/chromium/tools/depot_tools.git"

STPYV8_VERSION = STPYV8_V8_GIT_TAG

v8_deps_linux = os.environ.get('V8_DEPS_LINUX', '1') in ('1',)

if os.name in ("posix",):
    icu_data_folder = '/usr/share/stpyv8'
else:
    icu_data_folder = None

os.environ['PATH'] = "{}:{}".format(os.environ['PATH'], DEPOT_HOME)

gn_args = {
    # "v8_use_snapshot"              : "true",
    "v8_enable_disassembler": "false",
    "v8_enable_i18n_support": "true",
    "is_component_build": "false",
    "is_debug": "false",
    "use_custom_libcxx": "false",
    "v8_monolithic": "true",
    "v8_use_external_startup_data": "false"
}

debug_gn_args = {
    "is_debug": "true",
    "symbol_level": 2,
    "use_goma": "false",
    "goma_dir": "\"None\"",
    "v8_enable_backtrace": "true",
    "v8_enable_fast_mksnapshot": "true",
    "v8_enable_slow_dchecks": "true",
    "v8_optimized_debug": "false"
}

if STPYV8_DEBUG:
    gn_args.update(debug_gn_args)

GN_ARGS = ' '.join("{}={}".format(key, gn_args[key]) for key in gn_args)

source_files = ["Exception.cpp",
                "Platform.cpp",
                "Isolate.cpp",
                "Context.cpp",
                "Engine.cpp",
                "Wrapper.cpp",
                "WrapperCLJS.cpp",
                "Locker.cpp",
                "Utils.cpp",
                "STPyV8.cpp"]

define_macros = [("BOOST_PYTHON_STATIC_LIB", None)]
undef_macros = []
include_dirs = []
library_dirs = []
libraries = []
extra_compile_args = []
extra_link_args = []

include_dirs.append(os.path.join(V8_HOME, 'include'))
if not STPYV8_DEBUG:
    v8_target_path = "out.gn/x64.release.sample"
else:
    v8_target_path = "out.gn/x64.debug.sample"

library_dirs.append(os.path.join(V8_HOME, '{}/obj/'.format(v8_target_path)))


def get_libboost_python_name():
    ubuntu_platforms = ('ubuntu',)
    current_platform = platform.platform().lower()

    if any(p in current_platform for p in ubuntu_platforms):
        return "boost_python{}".format(sys.version_info.major)

    return "boost_python{}{}".format(sys.version_info.major, sys.version_info.minor)


if os.name in ("nt",):
    include_dirs += os.environ["INCLUDE"].split(';')
    library_dirs += os.environ["LIB"].split(';')
    libraries += ["winmm", "ws2_32"]
    extra_compile_args += ["/O2", "/GL", "/MT", "/EHsc", "/Gy", "/Zi"]
    extra_link_args += ["/DLL", "/OPT:REF", "/OPT:ICF", "/MACHINE:X86"]
elif os.name in ("posix",):
    libraries = ["boost_system", "v8_monolith", get_libboost_python_name()]
    extra_compile_args.append('-std=c++11')

    if platform.system() in ('Linux',):
        libraries.append("rt")

if STPYV8_DEBUG:
    undef_macros += ['NDEBUG']
    extra_compile_args += ["-g3", "-O0"]
