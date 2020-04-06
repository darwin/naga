#pragma once

class CPlatform;
typedef std::shared_ptr<CPlatform> CPlatformPtr;

class CContext;
typedef std::shared_ptr<CContext> CContextPtr;

class CIsolate;
typedef std::shared_ptr<CIsolate> CIsolatePtr;

class CScript;
typedef std::shared_ptr<CScript> CScriptPtr;

class CJavascriptStackTrace;
typedef std::shared_ptr<CJavascriptStackTrace> CJavascriptStackTracePtr;

class CJavascriptStackFrame;
typedef std::shared_ptr<CJavascriptStackFrame> CJavascriptStackFramePtr;

class CJSObject;
typedef std::shared_ptr<CJSObject> CJSObjectPtr;

class CJSObjectArray;
typedef std::shared_ptr<CJSObjectArray> CJSObjectArrayPtr;

class CJSObjectFunction;
typedef std::shared_ptr<CJSObjectFunction> CJSObjectFunctionPtr;

class CJSObjectNull;
typedef std::shared_ptr<CJSObjectNull> CJSObjectNullPtr;

class CJSObjectUndefined;
typedef std::shared_ptr<CJSObjectUndefined> CJSObjectUndefinedPtr;
