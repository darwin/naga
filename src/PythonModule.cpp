#include "PythonModule.h"
#include "PythonExpose.h"

PYBIND11_MODULE(naga_native, py_module) {
  useLogging();

  SPDLOG_INFO("=====================================================================================================");
  SPDLOG_INFO("Initializing naga_native module...");

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
}
