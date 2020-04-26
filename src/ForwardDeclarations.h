#ifndef NAGA_FORWARDDECLARATIONS_H_
#define NAGA_FORWARDDECLARATIONS_H_

#include "Base.h"

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

namespace v8 {

using IsolatePtr = v8::Isolate*; // __nonnull;
using TryCatchPtr = v8::TryCatch*; // __nonnull;

}  // namespace v8

namespace spdlog {

class logger;

}

using LoggerPtr = spdlog::logger*; // __nonnull;

#endif