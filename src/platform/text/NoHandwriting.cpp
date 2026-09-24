#include "platform/text/IHandwriting.hpp"

#include <memory>

namespace phvikapen::platform::text {

// Handwriting is read by Windows; everywhere else the application says it cannot read any.
std::unique_ptr<IHandwriting> openHandwriting() {
    return nullptr;
}

}
