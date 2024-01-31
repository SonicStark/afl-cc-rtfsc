#include "debug.h"

#include <libiberty/libiberty.h>

#if !DETECT_LEAKS
const char* __asan_default_options() { return "detect_leaks=0"; }
#endif

int main(int argc, char **argv) {

  expandargv(&argc, &argv);

  printf("===RESULT===" "\n");
  for (int i=1; i<argc; ++i) {
    printf("%s" "\n", argv[i]);
  }

}
