#include "app.h"
#include "components.h"

int main(int argc, char *argv[]) {
  pipeline::RunApp<pipeline::StdDecode>(argc, argv);
  return 0;
}
