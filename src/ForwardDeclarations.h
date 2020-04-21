#pragma once

#include "Base.h"

class CPlatform;
class CContext;
class CEngine;
class CIsolate;
class CScript;
class CJSStackTrace;
class CJSStackFrame;
class CJSException;
class CTracer;
class CHospital;
class CEternals;
class CJSObject;
class CJSObjectBase;
class CJSObjectGenericImpl;
class CJSObjectFunctionImpl;
class CJSObjectArrayImpl;
class CJSObjectCLJSImpl;
class CJSObjectAPI;

typedef std::shared_ptr<CContext> CContextPtr;
typedef std::shared_ptr<CIsolate> CIsolatePtr;
typedef std::shared_ptr<CScript> CScriptPtr;
typedef std::shared_ptr<CJSStackTrace> CJSStackTracePtr;
typedef std::shared_ptr<CJSStackFrame> CJSStackFramePtr;
typedef std::shared_ptr<CJSException> CJSExceptionPtr;
typedef std::shared_ptr<CJSObject> CJSObjectPtr;

namespace v8 {

typedef gsl::not_null<v8::Isolate*> IsolateRef;

}

namespace spdlog {

class logger;

}

typedef gsl::not_null<spdlog::logger*> LoggerRef;