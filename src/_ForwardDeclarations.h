#ifndef NAGA_FORWARDDECLARATIONS_H_
#define NAGA_FORWARDDECLARATIONS_H_

class CJSPlatform;
class CJSContext;
class CJSEngine;
class CJSIsolate;
class CJSScript;
class CJSStackTrace;
class CJSStackFrame;
class CJSException;
class CTracer;
class CJSHospital;
class CJSEternals;
class CJSObject;
class CJSObjectBase;
class CJSObjectGenericImpl;
class CJSObjectFunctionImpl;
class CJSObjectArrayImpl;
class CJSObjectCLJSImpl;
class CJSObjectAPI;

using CJSContextPtr = std::shared_ptr<CJSContext>;
using CJSIsolatePtr = std::shared_ptr<CJSIsolate>;
using CJSScriptPtr = std::shared_ptr<CJSScript>;
using CJSStackTracePtr = std::shared_ptr<CJSStackTrace>;
using CJSStackFramePtr = std::shared_ptr<CJSStackFrame>;
using CJSObjectPtr = std::shared_ptr<CJSObject>;

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