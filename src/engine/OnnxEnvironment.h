#pragma once

#include <onnxruntime_cxx_api.h>

namespace daw::onnx
{
/** The one ONNX Runtime environment all of DawIt's models share (Basic Pitch for audio to MIDI,
    and the DiffSinger voices of Sing this). Made on first use; lives until the app quits. */
Ort::Env& getEnvironment();
} // namespace daw::onnx
