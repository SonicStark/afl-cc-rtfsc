#include "afl-cc.h"

void add_no_builtin(aflcc_state_t *aflcc) {

  if (getenv("AFL_NO_BUILTIN") || getenv("AFL_LLVM_LAF_TRANSFORM_COMPARES") ||
      getenv("LAF_TRANSFORM_COMPARES") || getenv("AFL_LLVM_LAF_ALL") ||
      aflcc->lto_mode) {

    INSERT_PARAM(aflcc, "-fno-builtin-strcmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strncmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strcasecmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strncasecmp");
    INSERT_PARAM(aflcc, "-fno-builtin-memcmp");
    INSERT_PARAM(aflcc, "-fno-builtin-bcmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strstr");
    INSERT_PARAM(aflcc, "-fno-builtin-strcasestr");

  }

}
