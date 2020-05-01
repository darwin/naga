#ifndef NAGA_PYBINDNAGAMODULE_H_
#define NAGA_PYBINDNAGAMODULE_H_

#include "Base.h"
#include "Logging.h"
#include "PybindLogging.h"

namespace pybind11 {

// note we want to do private inheritance here so we don't miss any usage and cover all active uses with our wrapper
class naga_module : /* private */ module {
  using super = module;

 public:
  explicit naga_module(const module& py_module) : module(py_module) {}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
  template <typename Func, typename... Extra>
  naga_module& def(const char* name, Func&& f, Extra&&... extra) {
    auto module_name = PyModule_GetName(this->ptr());
    auto wf = wrapWithLogger(std::forward<Func>(f), name, module_name, "(module fn)");
    super::def(name, wf, std::forward<Extra>(extra)...);
    return *this;
  }
#pragma clang diagnostic pop
};

}  // namespace pybind11

#endif