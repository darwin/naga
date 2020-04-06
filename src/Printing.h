#pragma once

#include "Base.h"

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& obj);
std::ostream& operator<<(std::ostream& os, const CJSException& ex);
std::ostream& operator<<(std::ostream& os, const CJSObject& obj);
std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj);

std::ostream& operator<<(std::ostream& os, v8::Local<v8::Value> v8_val);