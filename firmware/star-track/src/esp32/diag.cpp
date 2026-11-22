#include "diag.h"

#include <esp_core_dump.h>
#include <esp_system.h>
#include <stdio.h>

namespace esp32 {

namespace {
char report[300] = "";
const char *code = "poweron";
bool fault = false;
}  // namespace

void readResetReason() {
  esp_reset_reason_t r = esp_reset_reason();
  switch (r) {
    case ESP_RST_POWERON: code = "poweron"; break;
    case ESP_RST_SW: code = "sw"; break;
    case ESP_RST_EXT: code = "ext"; break;
    case ESP_RST_BROWNOUT: code = "brownout"; break;
    case ESP_RST_PANIC: code = "panic"; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: code = "wdt"; break;
    default: code = "other"; break;
  }
  int n = 0;
  switch (r) {
    case ESP_RST_POWERON:
      return;
    case ESP_RST_SW:
      snprintf(report, sizeof report, "restarted by the firmware (after an update)");
      return;
    case ESP_RST_EXT:
      snprintf(report, sizeof report, "restarted by the reset button");
      return;
    case ESP_RST_BROWNOUT:
      n = snprintf(report, sizeof report,
                   "restarted after a brown-out: the ESP32's supply dropped too low (motor current, "
                   "supply current limit, 5 V converter)");
      break;
    case ESP_RST_PANIC:
      n = snprintf(report, sizeof report, "restarted after a crash");
      break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
      n = snprintf(report, sizeof report, "restarted by a watchdog: the firmware hung (reason %d)", (int)r);
      break;
    default:
      snprintf(report, sizeof report, "restarted (reset reason %d)", (int)r);
      return;
  }
  fault = true;
  if (r == ESP_RST_BROWNOUT) return;
  // The crash's place in the code: decode with addr2line and the firmware's .elf.
  esp_core_dump_summary_t s;
  if (esp_core_dump_get_summary(&s) == ESP_OK) {
    n += snprintf(report + n, sizeof report - n, " in task %s at 0x%08lx, backtrace", s.exc_task,
                  (unsigned long)s.exc_pc);
    for (uint32_t i = 0; i < s.exc_bt_info.depth && i < 8 && n < (int)sizeof report - 12; i++)
      n += snprintf(report + n, sizeof report - n, " 0x%08lx", (unsigned long)s.exc_bt_info.bt[i]);
  }
}

const char *resetReport() { return report; }

const char *resetCode() { return code; }

bool resetWasFault() { return fault; }

}  // namespace esp32
