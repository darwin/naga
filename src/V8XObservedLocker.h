#ifndef NAGA_V8OBSERVERVEDLOCKER_H_
#define NAGA_V8OBSERVERVEDLOCKER_H_

#include "Base.h"
#include "Logging.h"
#include "Printing.h"

namespace v8x {

class ObservedLocker : public v8::Locker {
 public:
  explicit ObservedLocker(v8::Isolate* v8_isolate) : v8::Locker(v8_isolate) {
    HTRACE(kIsolateLockingLogger, "ObservedLocker::ObservedLocker {} v8_isolate={}", THIS, P$(v8_isolate));
  }

  ~ObservedLocker() { HTRACE(kIsolateLockingLogger, "ObservedLocker::~ObservedLocker {}", THIS); }
};

}  // namespace v8x

#endif