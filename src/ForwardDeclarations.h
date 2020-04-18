#pragma once

class CPlatform;
class CContext;
class CEngine;
class CIsolate;
class CScript;
class CJSStackTrace;
class CJSStackFrame;
class CJSException;
class CJSObject;
class CJSObjectArray;
class CJSObjectCLJS;

typedef std::shared_ptr<CPlatform> CPlatformPtr;
typedef std::shared_ptr<CContext> CContextPtr;
typedef std::shared_ptr<CIsolate> CIsolatePtr;
typedef std::shared_ptr<CScript> CScriptPtr;
typedef std::shared_ptr<CJSStackTrace> CJSStackTracePtr;
typedef std::shared_ptr<CJSStackFrame> CJSStackFramePtr;
typedef std::shared_ptr<CJSException> CJSExceptionPtr;
typedef std::shared_ptr<CJSObject> CJSObjectPtr;
typedef std::shared_ptr<CJSObjectArray> CJSObjectArrayPtr;
typedef std::shared_ptr<CJSObjectCLJS> CJSObjectCLJSPtr;

namespace v8 {

typedef gsl::not_null<v8::Isolate*> IsolateRef;

}

namespace spdlog {

class logger;

}

typedef gsl::not_null<spdlog::logger*> LoggerRef;