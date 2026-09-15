#pragma once

namespace phvikapen::platform::update {

/// Runs the installer and updater startup logic.
///
/// Must be the first call in main(), before any other initialization. When the process was
/// launched by the installer or updater, this may run the corresponding hook and then exit or
/// restart the process. Otherwise it returns immediately.
void runStartupHook();

} // namespace phvikapen::platform::update
