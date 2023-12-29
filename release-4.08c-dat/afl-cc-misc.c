#include "afl-cc.h"

void add_misc_args(aflcc_state_t *aflcc) {

  if (!aflcc->have_pic) { INSERT_PARAM(aflcc, "-fPIC"); }

  if (getenv("AFL_HARDEN")) {

    INSERT_PARAM(aflcc, "-fstack-protector-all");

    if (!aflcc->fortify_set) ctrl_fortification(aflcc, 2);

  }

  if (!getenv("AFL_DONT_OPTIMIZE")) {

    INSERT_PARAM(aflcc, "-g");
    if (!aflcc->have_o)   INSERT_PARAM(aflcc, "-O3");
    if (!aflcc->have_unroll)  INSERT_PARAM(aflcc, "-funroll-loops");
    // if (strlen(aflcc->march_opt) > 1 && aflcc->march_opt[0] == '-')
    //     INSERT_PARAM(aflcc, aflcc->march_opt);

  }

  if (aflcc->x_set) {

    INSERT_PARAM(aflcc, "-x");
    INSERT_PARAM(aflcc, "none");

  }

}