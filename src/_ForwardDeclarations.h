#ifndef NAGA_FORWARDDECLARATIONS_H_
#define NAGA_FORWARDDECLARATIONS_H_

class JSPlatform;
class JSContext;
class JSEngine;
class JSIsolate;
class JSScript;
class JSStackTrace;
class JSStackTraceIterator;
class JSStackFrame;
class JSException;
class JSTracer;
class JSHospital;
class JSEternals;
class JSObject;
class JSObjectBase;
class JSObjectGenericImpl;
class JSObjectFunctionImpl;
class JSObjectArrayImpl;
class JSObjectCLJSImpl;
class JSObjectAPI;

using SharedJSContextPtr = std::shared_ptr<JSContext>;
using SharedJSIsolatePtr = std::shared_ptr<JSIsolate>;
using SharedJSScriptPtr = std::shared_ptr<JSScript>;
using SharedJSStackTracePtr = std::shared_ptr<JSStackTrace>;
using SharedConstJSStackTracePtr = std::shared_ptr<const JSStackTrace>;
using SharedJSStackTraceIteratorPtr = std::shared_ptr<JSStackTraceIterator>;
using SharedConstJSStackTraceIteratorPtr = std::shared_ptr<const JSStackTraceIterator>;
using SharedJSStackFramePtr = std::shared_ptr<JSStackFrame>;
using SharedJSObjectPtr = std::shared_ptr<JSObject>;

namespace v8x {

class LockedIsolatePtr;
class ProtectedIsolatePtr;

using TryCatchPtr = v8::TryCatch*;
using SharedIsolateLockerPtr = std::shared_ptr<v8::Locker>;
using WeakIsolateLockerPtr = std::weak_ptr<v8::Locker>;

}  // namespace v8x

namespace spdlog {

class logger;

}

using LoggerPtr = spdlog::logger*;

#endif