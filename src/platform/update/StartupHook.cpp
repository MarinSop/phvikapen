#include "platform/update/StartupHook.hpp"

#include <Velopack.hpp>

namespace phvikapen::platform::update {

void runStartupHook() {
    Velopack::VelopackApp::Build().Run();
}

} // namespace phvikapen::platform::update
