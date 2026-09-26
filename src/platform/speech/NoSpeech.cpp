#include "platform/speech/ISpeech.hpp"

#include <memory>

namespace phvikapen::platform::speech {

// No machine the application runs on reads speech yet, so it says so rather than pretending.
std::unique_ptr<ISpeech> openSpeech() {
    return nullptr;
}

}
