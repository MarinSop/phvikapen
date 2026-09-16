#pragma once

namespace phvikapen::app {

/// Starts logging into a rotating file below QStandardPaths::AppLocalDataLocation and forwards
/// Qt's own messages there as well.
///
/// Call once, after the application name has been set, so that the log lands in the right place.
void initializeLogging();

} // namespace phvikapen::app
