#pragma once

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

typedef std::shared_ptr<CJSContext> CJSContextPtr;
typedef std::shared_ptr<CJSIsolate> CJSIsolatePtr;
typedef std::shared_ptr<CJSScript> CJSScriptPtr;
typedef std::shared_ptr<CJSStackTrace> CJSStackTracePtr;
typedef std::shared_ptr<CJSStackFrame> CJSStackFramePtr;
typedef std::shared_ptr<CJSObject> CJSObjectPtr;

namespace v8 {

using IsolateRef = v8::Isolate* __nonnull;

}

namespace spdlog {

class logger;

}

typedef gsl::not_null<spdlog::logger*> LoggerRef;