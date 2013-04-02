#include "silent.h"

namespace pipeline {
bool SILENT = false;

void SetSilent(bool v) {
  SILENT = v;
}
} // namespace pipeline
