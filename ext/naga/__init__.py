# This is a pure Python wrapper of the naga_native module implemented in C++ code present in src
# This module is presents public API for naga library users

import os
import re

if os.environ.get('NAGA_WAIT_FOR_DEBUGGER') is not None:
    input("Waiting for debugger, please hit ENTER when ready...")

# noinspection PyUnresolvedReferences
import naga_native

__version__ = '0.1'

__all__ = ["JSError",
           "JSObject",
           "JSNull",
           "JSUndefined",
           "JSArray",
           "JSFunction",
           "JSClass",
           "JSEngine",
           "JSContext",
           "JSIsolate",
           "JSStackTrace",
           "JSStackFrame",
           "JSScript",
           "JSLocker",
           "JSUnlocker",
           "JSPlatform"]


class JSError(Exception):
    def __init__(self, js_exception):
        Exception.__init__(self)
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


JSObject = naga_native.JSObject
JSArray = naga_native.JSObject  # for backward compatibility
JSFunction = naga_native.JSObject  # for backward compatibility
JSPlatform = naga_native.JSPlatform
JSUndefined = naga_native.JSUndefined
JSNull = naga_native.JSNull


class JSLocker(naga_native.JSLocker):
    def __enter__(self):
        self.enter()

        if JSContext.entered:
            self.leave()
            raise RuntimeError("Lock should be acquired before enter the context")

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if JSContext.entered:
            self.leave()
            raise RuntimeError("Lock should be released after leave the context")

        self.leave()

    def __bool__(self):
        return self.entered()


class JSUnlocker(naga_native.JSUnlocker):
    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

    def __bool__(self):
        return self.entered()


# noinspection PyPep8Naming
class JSClass(object):
    __properties__ = {}
    __watchpoints__ = {}

    def __getattr__(self, name):
        if name == 'constructor':
            return JSClassConstructor(self.__class__)

        if name == 'prototype':
            return JSClassPrototype(self.__class__)

        prop = self.__dict__.setdefault('__properties__', {}).get(name, None)

        if prop and callable(prop[0]):
            return prop[0]()

        raise AttributeError(name)

    def __setattr__(self, name, value):
        prop = self.__dict__.setdefault('__properties__', {}).get(name, None)

        if prop and callable(prop[1]):
            return prop[1](value)

        return object.__setattr__(self, name, value)

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

    def __defineGetter__(self, name, getter):
        """Binds an object's property to a function to be called when that property is looked up."""
        self.__properties__[name] = (getter, self.__lookupSetter__(name))

    def __lookupGetter__(self, name):
        """Return the function bound as a getter to the specified property."""
        return self.__properties__.get(name, (None, None))[0]

    def __defineSetter__(self, name, setter):
        """Binds an object's property to a function to be called when an attempt is made to set that property."""
        self.__properties__[name] = (self.__lookupGetter__(name), setter)

    def __lookupSetter__(self, name):
        """Return the function bound as a setter to the specified property."""
        return self.__properties__.get(name, (None, None))[1]

    def watch(self, prop, handler):
        """Watches for a property to be assigned a value and runs a function when that occurs."""
        self.__watchpoints__[prop] = handler

    def unwatch(self, prop):
        """Removes a watchpoint set with the watch method."""
        del self.__watchpoints__[prop]


class JSClassConstructor(JSClass):
    def __init__(self, cls):
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
        self.cls = cls

    @property
    def constructor(self):
        return JSClassConstructor(self.cls)

    @property
    def name(self):
        return self.cls.__name__


class JSEngine(naga_native.JSEngine):
    def __init__(self):
        naga_native.JSEngine.__init__(self)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        del self


JSScript = naga_native.JSScript
JSStackTrace = naga_native.JSStackTrace
JSStackTrace.Options = naga_native.JSStackTraceOptions
JSStackTrace.GetCurrentStackTrace = staticmethod(
    lambda frame_limit, options: naga_native.JSIsolate.current.GetCurrentStackTrace(frame_limit, options))
JSStackFrame = naga_native.JSStackFrame


class JSIsolate(naga_native.JSIsolate):
    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()
        del self


class JSContext(naga_native.JSContext):
    def __init__(self, obj=None, ctxt=None):
        if JSLocker.active:
            self.lock = JSLocker()
            self.lock.enter()

        if ctxt:
            naga_native.JSContext.__init__(self, ctxt)
        else:
            naga_native.JSContext.__init__(self, obj)

    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

        if hasattr(JSLocker, 'lock'):
            self.lock.leave()
            self.lock = None

        del self


v8_default_platform = JSPlatform.instance
v8_default_platform.init()

v8_default_isolate = JSIsolate()
v8_default_isolate.enter()
