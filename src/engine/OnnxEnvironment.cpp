#include "OnnxEnvironment.h"

namespace daw::onnx
{
Ort::Env& getEnvironment()
{
    static Ort::Env env (ORT_LOGGING_LEVEL_WARNING, "DawIt");
    return env;
}
} // namespace daw::onnx
