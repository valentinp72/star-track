// Why the board started: a power-on, an update, the reset button, or a fault
// (brown-out, crash, watchdog). Crashes also leave a core dump in flash.
#pragma once

namespace esp32 {

// Reads the reset reason (and the core dump's summary after a crash). Call once
// at boot.
void readResetReason();
// A sentence for the console and the app; empty after a normal power-on.
const char *resetReport();
// True after a brown-out, a crash or a watchdog reset.
bool resetWasFault();

}  // namespace esp32
