#include "PythonModule.h"
#include "PythonExpose.h"

py::module g_naga_native_module;

py::module& getNagaNativeModule() {
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
}
