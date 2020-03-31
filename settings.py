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
build_config = "debug" if STPYV8_DEBUG else "release"
v8_target_path = "out.gn-{}/x64.{}.stpyv8".format(STPYV8_VERSION, build_config)

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

# I took this from ninja files of the V8 debug build, we have to match them to prevent crashes in debug builds
# It also helps with debugging
debug_macros = [
    ("_LIBCPP_HAS_NO_ALIGNED_ALLOCATION", None),
    ("__STDC_CONSTANT_MACROS", None),
    ("__STDC_FORMAT_MACROS", None),
    ("__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES", "0"),
    ("_DEBUG", None),
    ("DYNAMIC_ANNOTATIONS_ENABLED", "1"),
    ("V8_TYPED_ARRAY_MAX_SIZE_IN_HEAP", "64"),
    ("ENABLE_GDB_JIT_INTERFACE", None),
    ("ENABLE_MINOR_MC", None),
    ("OBJECT_PRINT", None),
    ("VERIFY_HEAP", None),
    ("V8_TRACE_MAPS", None),
    ("V8_ENABLE_ALLOCATION_TIMEOUT", None),
    ("V8_ENABLE_FORCE_SLOW_PATH", None),
    ("V8_ENABLE_DOUBLE_CONST_STORE_CHECK", None),
    ("V8_INTL_SUPPORT", None),
    ("ENABLE_HANDLE_ZAPPING", None),
    ("V8_SNAPSHOT_NATIVE_CODE_COUNTERS", None),
    ("V8_CONCURRENT_MARKING", None),
    ("V8_ARRAY_BUFFER_EXTENSION", None),
    ("V8_ENABLE_LAZY_SOURCE_POSITIONS", None),
    ("V8_CHECK_MICROTASKS_SCOPES_CONSISTENCY", None),
    ("V8_EMBEDDED_BUILTINS", None),
    ("V8_WIN64_UNWINDING_INFO", None),
    ("V8_ENABLE_REGEXP_INTERPRETER_THREADED_DISPATCH", None),
    ("V8_SNAPSHOT_COMPRESSION", None),
    ("V8_ENABLE_CHECKS", None),
    ("V8_COMPRESS_POINTERS", None),
    ("V8_31BIT_SMIS_ON_64BIT_ARCH", None),
    ("V8_DEPRECATION_WARNINGS", None),
    ("V8_IMMINENT_DEPRECATION_WARNINGS", None),
    ("V8_TARGET_ARCH_X64", None),
    ("V8_HAVE_TARGET_OS", None),
    ("V8_TARGET_OS_MACOSX", None),
    ("DEBUG", None),
    ("ENABLE_SLOW_DCHECKS", None),
    ("DISABLE_UNTRUSTED_CODE_MITIGATIONS", None),
    ("U_USING_ICU_NAMESPACE", "0"),
    ("U_ENABLE_DYLOAD", "0"),
    ("USE_CHROMIUM_ICU", "1"),
    ("U_ENABLE_TRACING", "1"),
    ("U_ENABLE_RESOURCE_TRACING", "0"),
    ("U_STATIC_IMPLEMENTATION", None),
    #    ("ICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE", None),
    ("UCHAR_TYPE", "uint16_t"),
    ("V8_ENABLE_CHECKS", None),
    ("V8_COMPRESS_POINTERS", None),
    ("V8_31BIT_SMIS_ON_64BIT_ARCH", None),
    ("V8_DEPRECATION_WARNINGS", None),
    ("V8_IMMINENT_DEPRECATION_WARNINGS", None)
]

if STPYV8_DEBUG:
    define_macros += debug_macros;
