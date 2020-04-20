#include "Module.h"
#include "PythonExpose.h"

PYBIND11_MODULE(_STPyV8, py_module) {
  useLogging();

  SPDLOG_INFO("");
  SPDLOG_INFO("=====================================================================================================");
  SPDLOG_INFO("Initializing _STPyV8 module...");

  exposeAux(py_module);
  exposeJSNull(py_module);
  exposeJSUndefined(py_module);
  exposeJSObject(py_module);
  exposeJSToolkit(py_module);
  exposeJSPlatform(py_module);
  exposeJSIsolate(py_module);
  exposeJSStackFrame(py_module);
  exposeJSStackTrace(py_module);
  exposeJSError(py_module);
  exposeJSContext(py_module);
  exposeJSScript(py_module);
  exposeJSEngine(py_module);
  exposeJSLocker(py_module);
  exposeJSUnlocker(py_module);
}
