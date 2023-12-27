#include "afl-cc.h"

/* For the input -fsanitize=..., it:

  1. may have various OOB traps :) if ... doesn't contain ',' or 
    the input has bad syntax such as "-fsanitize=,"
  2. strips any fuzzer* in ... and writes back (may result in "-fsanitize=")
  3. rets 1 if exactly "fuzzer" found, otherwise rets 0
 */
static u8 parse_fsanitize(char *string) {

  u8 detect_single_fuzzer = 0;

  char *p, *ptr = string + strlen("-fsanitize=");
  char *new = ck_alloc(strlen(string) + 1); // ck_alloc will check alloc failure
  char *tmp = ck_alloc(strlen(ptr));
  u32   count = 0, len, ende = 0;

  strcpy(new, "-fsanitize=");

  do {

    p = strchr(ptr, ',');
    if (!p) {

      p = ptr + strlen(ptr) + 1;
      ende = 1;

    }

    len = p - ptr;
    if (len) {

      strncpy(tmp, ptr, len);
      tmp[len] = 0;
      // fprintf(stderr, "Found: %s\n", tmp);
      ptr += len + 1;
      if (*tmp) {

        u32 copy = 1;
        if (!strcmp(tmp, "fuzzer")) {

          detect_single_fuzzer = 1;
          copy = 0;

        } else if (!strncmp(tmp, "fuzzer", 6)) {

          copy = 0;

        }

        if (copy) {

          if (count) { strcat(new, ","); }
          strcat(new, tmp);
          ++count;

        }

      }

    } else {

      ptr++;                                    /*fprintf(stderr, "NO!\n"); */

    }

  } while (!ende);

  strcpy(string, new);
  // fprintf(stderr, "string: %s\n", string);
  // fprintf(stderr, "new: %s\n", new);

  ck_free(tmp);
  ck_free(new);

  return detect_single_fuzzer;

}

param_st handle_fsanitize(aflcc_state_t *aflcc, u8 *cur_argv, u8 scan) {

  param_st final_ = PARAM_MISS;

  if (!strncmp(cur_argv, "-fsanitize-coverage-", 20) && strstr(cur_argv, "list=")) {

    if (scan) {

      aflcc->have_instr_list = 1;
      final_ = PARAM_SCAN;

    } else {

      final_ = PARAM_ORIG; // may be set to DROP next

    }

  }

  if (!strcmp(cur_argv, "-fsanitize=fuzzer")) {

    if (scan) {

      aflcc->need_aflpplib = 1;
      final_ = PARAM_SCAN;

    } else {
      
      final_ = PARAM_DROP;

    }

  } else if (!strncmp(cur_argv, "-fsanitize=", strlen("-fsanitize=")) &&
      strchr(cur_argv, ',') &&  !strchr(cur_argv, '=,')) { // avoid OOB errors

    if (scan) {

      u8 *cur_argv_ = ck_strdup(cur_argv);

      if (parse_fsanitize(cur_argv_)) {

        aflcc->need_aflpplib = 1;
        final_ = PARAM_SCAN;

      }

      ck_free(cur_argv_);

    } else {

      parse_fsanitize(cur_argv);
      if (!cur_argv || strlen(cur_argv) <= strlen("-fsanitize="))
        final_ = PARAM_DROP; // this means it only has "fuzzer" previously.

    }

  } else if ((!strncmp(cur_argv, "-fsanitize=fuzzer-",
                          strlen("-fsanitize=fuzzer-")) ||
              !strncmp(cur_argv, "-fsanitize-coverage",
                          strlen("-fsanitize-coverage"))) &&
              (strncmp(cur_argv, "sanitize-coverage-allow",
                          strlen("sanitize-coverage-allow")) &&
              strncmp(cur_argv, "sanitize-coverage-deny",
                         strlen("sanitize-coverage-deny")) &&
              aflcc->instrument_mode != INSTRUMENT_LLVMNATIVE)) {

    if (scan) {

      final_ = PARAM_SCAN;

    } else {

      if (!be_quiet) { WARNF("Found '%s' - stripping!", cur_argv); }
      final_ = PARAM_DROP;

    }

  }

  if (!strcmp(cur_argv, "-fsanitize=address") || 
      !strcmp(cur_argv, "-fsanitize=memory")) {

    if (scan) {

      // "-fsanitize=undefined,address" may be un-treated, but it's OK.
      aflcc->asan_set = 1;
      final_ = PARAM_SCAN;

    } else {

      // It's impossible that final_ is PARAM_DROP before,
      // so no checks are needed here.
      final_ = PARAM_ORIG;

    }

  }

  if (final_ == PARAM_ORIG)
    INSERT_PARAM(aflcc, cur_argv);
  
  return final_;

}

void add_sanitizers(aflcc_state_t *aflcc, char **envp) {

  if (!aflcc->asan_set) {

    if (getenv("AFL_USE_ASAN")) {

      if (getenv("AFL_USE_MSAN")) FATAL("ASAN and MSAN are mutually exclusive");

      if (getenv("AFL_HARDEN"))
        FATAL("ASAN and AFL_HARDEN are mutually exclusive");

      set_fortification(aflcc, 0);
      INSERT_PARAM(aflcc, "-fsanitize=address");

    } else if (getenv("AFL_USE_MSAN")) {

      if (getenv("AFL_USE_ASAN")) FATAL("ASAN and MSAN are mutually exclusive");

      if (getenv("AFL_HARDEN"))
        FATAL("MSAN and AFL_HARDEN are mutually exclusive");

      set_fortification(aflcc, 0);
      INSERT_PARAM(aflcc, "-fsanitize=memory");

    }

  }

  if (getenv("AFL_USE_UBSAN")) {

    INSERT_PARAM(aflcc, "-fsanitize=undefined");
    INSERT_PARAM(aflcc, "-fsanitize-undefined-trap-on-error");
    INSERT_PARAM(aflcc, "-fno-sanitize-recover=all");
    INSERT_PARAM(aflcc, "-fno-omit-frame-pointer");

  }

  if (getenv("AFL_USE_TSAN")) {

    INSERT_PARAM(aflcc, "-fsanitize=thread");
    INSERT_PARAM(aflcc, "-fno-omit-frame-pointer");

  }

  if (getenv("AFL_USE_LSAN")) {

    INSERT_PARAM(aflcc, "-fsanitize=leak");
    add_lsan_ctrl(aflcc);

  }

  if (getenv("AFL_USE_CFISAN")) {

    if (aflcc->compiler_mode == GCC_PLUGIN || 
        aflcc->compiler_mode == GCC) {

      INSERT_PARAM(aflcc, "-fcf-protection=full");

    } else {

      if (!aflcc->lto_mode) {

        uint32_t i = 0, found = 0;
        while (envp[i] != NULL && !found)
          if (strncmp("-flto", envp[i++], 5) == 0) found = 1;
        if (!found) INSERT_PARAM(aflcc, "-flto");

      }

      INSERT_PARAM(aflcc, "-fsanitize=cfi");
      INSERT_PARAM(aflcc, "-fvisibility=hidden");

    }

  }

}
