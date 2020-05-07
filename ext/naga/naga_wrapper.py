# This module implements public API for naga library users
# This is a pure Python wrapper of the naga_native module implemented in C++
# It is imported in __init__ of the `naga` package.

import re
import atexit

import naga.config
import naga.debug_support
# noinspection PyUnresolvedReferences
import naga_native

__all__ = ["JSClass",
           "JSContext",
           "JSEngine",
           "JSError",
           "JSIsolate",
           "JSNull",
           "JSObject",
           "JSPlatform",
           "JSScript",
           "JSStackTrace",
           "JSStackFrame",
           "JSUndefined"]

if naga.config.naga_keep_backward_compatibility:
    __all__ += ["JSArray",
                "JSFunction"]


# noinspection PyPep8Naming
class JSClass(object):

    @property
    def constructor(self):
        return JSClassConstructor(self.__class__)

    @property
    def prototype(self):
        return JSClassPrototype(self.__class__)

    def toString(self):
        """Returns a string representation of an object."""
        return "[object %s]" % self.__class__.__name__

    def toLocaleString(self):
        """Returns a value as a string value appropriate to the host environment's current locale."""
        return self.toString()

    def valueOf(self):
        """Returns the primitive value of the specified object."""
        return self

    def hasOwnProperty(self, name):
        """Returns a Boolean value indicating whether an object has a property with the specified name."""
        return hasattr(self, name)

    # noinspection PyMethodMayBeStatic
    def isPrototypeOf(self, obj):
        """Returns a Boolean value indicating whether an object exists in the prototype chain of another object."""
        return False


class JSClassConstructor(JSClass):
    def __init__(self, cls):
        super().__init__()
        self.cls = cls

    @property
    def name(self):
        return self.cls.__name__

    def toString(self):
        return "function %s() {\n  [native code]\n}" % self.name

    def __call__(self, *args, **kwds):
        return self.cls(*args, **kwds)


class JSClassPrototype(JSClass):

    def __init__(self, cls):
        super().__init__()
        self.cls = cls

    @property
    def constructor(self):
        return JSClassConstructor(self.cls)

    @property
    def name(self):
        return self.cls.__name__


class JSError(Exception):
    def __init__(self, js_exception):
        super().__init__()
        self._impl = js_exception

    def __str__(self):
        return str(self._impl)

    def __getattribute__(self, attr):
        impl = super(JSError, self).__getattribute__("_impl")

        try:
            return getattr(impl, attr)
        except AttributeError:
            return super(JSError, self).__getattribute__(attr)

    RE_FRAME = re.compile(r"\s+at\s(?:new\s)?(?P<func>.+)\s\((?P<file>[^:]+):?(?P<row>\d+)?:?(?P<col>\d+)?\)")
    RE_FUNC = re.compile(r"\s+at\s(?:new\s)?(?P<func>.+)\s\((?P<file>[^)]+)\)")
    RE_FILE = re.compile(r"\s+at\s(?P<file>[^:]+):?(?P<row>\d+)?:?(?P<col>\d+)?")

    @staticmethod
    def parse_stack(value):
        stack = []

        def int_or_nul(v):
            return int(v) if v else None

        for line in value.split('\n')[1:]:
            m = JSError.RE_FRAME.match(line)

            if m:
                stack.append((m.group('func'), m.group('file'), int_or_nul(m.group('row')), int_or_nul(m.group('col'))))
                continue

            m = JSError.RE_FUNC.match(line)

            if m:
                stack.append((m.group('func'), m.group('file'), None, None))
                continue

            m = JSError.RE_FILE.match(line)

            if m:
                stack.append((None, m.group('file'), int_or_nul(m.group('row')), int_or_nul(m.group('col'))))
                continue

            assert line

        return stack

    @property
    def frames(self):
        return self.parse_stack(self.stackTrace)


class JSEngine(naga_native.JSEngine):
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        del self


class JSIsolate(naga_native.JSIsolate):
    def __enter__(self):
        self.lock()
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()
        self.unlock()
        del self


class JSContext(naga_native.JSContext):
    def __init__(self, global_scope=None):
        super().__init__(global_scope)

    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()
        del self


class JSStackTrace(naga_native.JSStackTrace):
    Options = naga_native.JSStackTraceOptions


# -- enhance naga_native module ---------------------------------------------------------------------------------------

# some exception-handling C++ code expects existence of "JSError" in naga_native module
naga_native.JSError = JSError

# -- expose some native objects directly ------------------------------------------------------------------------------

JSNull = naga_native.JSNull
JSUndefined = naga_native.JSUndefined
JSObject = naga_native.JSObject
JSPlatform = naga_native.JSPlatform
JSScript = naga_native.JSScript
JSStackFrame = naga_native.JSStackFrame

if naga.config.naga_keep_backward_compatibility:
    JSArray = naga_native.JSObject
    JSFunction = naga_native.JSObject

# -- init code --------------------------------------------------------------------------------------------------------

v8_default_platform: JSPlatform
v8_default_isolate: JSIsolate


def init_default_platform():
    global v8_default_platform
    v8_default_platform = JSPlatform.instance
    v8_default_platform.init()


def deinit_default_isolate():
    global v8_default_isolate
    v8_default_isolate.leave()
    v8_default_isolate.unlock()


def init_default_isolate():
    global v8_default_isolate
    v8_default_isolate = JSIsolate()
    v8_default_isolate.lock()
    v8_default_isolate.enter()
    atexit.register(deinit_default_isolate)


def init():
    init_default_platform()
    init_default_isolate()


if naga.config.naga_auto_init:
    init()
