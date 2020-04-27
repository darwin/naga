#include "PythonModule.h"
#include "PythonExpose.h"
#include "Logging.h"

py::module g_naga_native_module;

py::module& getNagaNativeModule() {
  assert((bool)g_naga_native_module);
  return g_naga_native_module;
}

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonModuleLogger), __VA_ARGS__)

PYBIND11_MODULE(naga_native, py_module) {
  useLogging();

  TRACE("=====================================================================================================");
  TRACE("Initializing naga_native module...");

  exposeAux(py_module);
  exposeToolkit(py_module);

  exposeJSNull(py_module);
  exposeJSUndefined(py_module);
  exposeJSObject(py_module);
  exposeJSPlatform(py_module);
  exposeJSIsolate(py_module);
  exposeJSStackFrame(py_module);
  exposeJSStackTrace(py_module);
  exposeJSException(py_module);
  exposeJSContext(py_module);
  exposeJSScript(py_module);
  exposeJSEngine(py_module);
  exposeJSLocker(py_module);
  exposeJSUnlocker(py_module);

  g_naga_native_module = py_module;
  // we have to make sure this global variable gets cleared before Python interpreter gets deinitialized
  // https://pybind11.readthedocs.io/en/stable/advanced/misc.html?highlight=atexit#module-destructors
  auto atexit = py::module::import("atexit");
  atexit.attr("register")(py::cpp_function([] {
    TRACE("Deinitializing naga_native module...");
    g_naga_native_module = py::none();
  }));
}
