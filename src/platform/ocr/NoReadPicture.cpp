#include "platform/ocr/IReadPicture.hpp"

#include <memory>

namespace phvikapen::platform::ocr {

// The words in a picture are read by the machine the application runs on; where it carries no
// reader, the application says so rather than pretending.
std::unique_ptr<IReadPicture> openReadPicture() {
    return nullptr;
}

}
