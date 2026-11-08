#include "OnnxEnvironment.h"

#include <cstdlib>
#include <stdexcept>

#ifdef _WIN32
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

namespace daw::onnx
{
Ort::Env& getEnvironment()
{
    static Ort::Env env = []
    {
        // These must be set before ONNX creates its platform environment. On Windows,
        // the pinned runtime only prevents ETW registration through its test signal;
        // its public disable API alone can miss startup events or be reversed by ETW.
       #ifdef _WIN32
        if (! SetEnvironmentVariableW (L"ORT_RUNNING_UNIT_TESTS", L"1"))
            throw std::runtime_error ("Could not disable ONNX Runtime telemetry");
       #else
        if (setenv ("ORT_DISABLE_TELEMETRY", "1", 1) != 0)
            throw std::runtime_error ("Could not disable ONNX Runtime telemetry");
       #endif

        Ort::Env result (ORT_LOGGING_LEVEL_WARNING, "DawIt");
        result.DisableTelemetryEvents();
        return result;
    }();
    return env;
}
} // namespace daw::onnx
