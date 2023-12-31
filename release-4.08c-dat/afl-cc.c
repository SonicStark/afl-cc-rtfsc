#include "afl-cc.h"

/* Process params passed to main()
  - If scan is true, it only reads them and set some variables in aflcc.
  - Otherwise it reads each param, decides to keep or drop it. */
static void process_params(aflcc_state_t *aflcc, u8 scan,
                            u32 argc, char **argv) {

  limit_params(aflcc, argc);

  // for (u32 x = 0; x < argc; ++x) fprintf(stderr, "[%u] %s\n", x, argv[x]);

  /* Process the argument list. */

  u8 skip_next = 0;

  while (--argc) {

    u8 *cur = *(++argv);

    if (skip_next > 0) {

      skip_next--;
      continue;

    }

    if (PARAM_MISS != handle_misc_args(aflcc, cur, scan))
      continue;
    
    if (PARAM_MISS != handle_fsanitize(aflcc, cur, scan))
      continue;

    if (PARAM_MISS != handle_linking_args(aflcc, cur, scan, 
                                          &skip_next, argv))
      continue;

    if (*cur == '@') {

      /* response file support
       we have two choices - move everything to the command line or
       rewrite the response files to temporary files and delete them
       afterwards. We choose the first for easiness.
       We do *not* support quotes in the rsp files to cope with spaces in
       filenames etc! If you need that then send a patch! */

      u8 *filename = cur + 1;
      if (aflcc->debug) { DEBUGF("response file=%s\n", filename); }
      FILE       *f = fopen(filename, "r");
      struct stat st;

      // Check not found or empty? let the compiler complain if so.
      if (!f || fstat(fileno(f), &st) < 0 || st.st_size < 1) {

        if (!scan) insert_param(aflcc, cur);
        continue;

      }

      u8    *tmpbuf = malloc(st.st_size + 2), *ptr;
      char **args = malloc(sizeof(char *) * (st.st_size >> 1));
      int    count = 1, cont = 0, cont_act = 0;

      while (fgets(tmpbuf, st.st_size + 1, f)) {

        ptr = tmpbuf;
        // fprintf(stderr, "1: %s\n", ptr);
        //  no leading whitespace
        while (isspace(*ptr)) {

          ++ptr;
          cont_act = 0;

        }

        // no comments, no empty lines
        if (*ptr == '#' || *ptr == '\n' || !*ptr) { continue; }
        // remove LF
        if (ptr[strlen(ptr) - 1] == '\n') { ptr[strlen(ptr) - 1] = 0; }
        // remove CR
        if (*ptr && ptr[strlen(ptr) - 1] == '\r') { ptr[strlen(ptr) - 1] = 0; }
        // handle \ at end of line
        if (*ptr && ptr[strlen(ptr) - 1] == '\\') {

          cont = 1;
          ptr[strlen(ptr) - 1] = 0;

        }

        // fprintf(stderr, "2: %s\n", ptr);

        // remove whitespace at end
        while (*ptr && isspace(ptr[strlen(ptr) - 1])) {

          ptr[strlen(ptr) - 1] = 0;
          cont = 0;

        }

        // fprintf(stderr, "3: %s\n", ptr);
        if (*ptr) {

          do {

            u8 *value = ptr;
            while (*ptr && !isspace(*ptr)) {

              ++ptr;

            }

            while (*ptr && isspace(*ptr)) {

              *ptr++ = 0;

            }

            if (cont_act) {

              u32 len = strlen(args[count - 1]) + strlen(value) + 1;
              u8 *tmp = malloc(len);
              snprintf(tmp, len, "%s%s", args[count - 1], value);
              free(args[count - 1]);
              args[count - 1] = tmp;
              cont_act = 0;

            } else {

              args[count++] = strdup(value);

            }

          } while (*ptr);

        }

        if (cont) {

          cont_act = 1;
          cont = 0;

        }

      }

      if (count) { process_params(aflcc, scan, count, args); }

      // we cannot free args[] unless we don't need
      // to keep any reference in cc_params
      if (scan) {
        if (count)
          do { free(args[--count]); } while (count);
        free(args);
      }
      free(tmpbuf);

      continue;

    }

    if (!scan)
      insert_param(aflcc, cur);

  }

}

/* Copy argv to cc_params, making the necessary edits. */

static void edit_params(aflcc_state_t *aflcc, u32 argc, char **argv, char **envp) {

  set_real_argv0(aflcc);

  // prevent unnecessary build errors
  if (aflcc->compiler_mode != GCC_PLUGIN && 
      aflcc->compiler_mode != GCC) {

    insert_param(aflcc, "-Wno-unused-command-line-argument");

  }

  if (aflcc->compiler_mode == GCC || 
      aflcc->compiler_mode == CLANG) {

    add_assembler(aflcc);

  }

  if (aflcc->compiler_mode == GCC_PLUGIN) {

    add_gcc_plugin(aflcc);

  }

  if (aflcc->compiler_mode == LLVM || 
      aflcc->compiler_mode == LTO) {

    if (aflcc->lto_mode && aflcc->have_instr_env) {

      load_llvm_pass(aflcc, "afl-llvm-lto-instrumentlist.so");

    }

    if (getenv("AFL_LLVM_DICT2FILE")) {

      load_llvm_pass(aflcc, "afl-llvm-dict2file.so");

    }

    // laf
    if (getenv("LAF_SPLIT_SWITCHES") || getenv("AFL_LLVM_LAF_SPLIT_SWITCHES")) {

      load_llvm_pass(aflcc, "split-switches-pass.so");

    }

    if (getenv("LAF_TRANSFORM_COMPARES") ||
        getenv("AFL_LLVM_LAF_TRANSFORM_COMPARES")) {

      load_llvm_pass(aflcc, "compare-transform-pass.so");

    }

    if (getenv("LAF_SPLIT_COMPARES") || getenv("AFL_LLVM_LAF_SPLIT_COMPARES") ||
        getenv("AFL_LLVM_LAF_SPLIT_FLOATS")) {

      load_llvm_pass(aflcc, "split-compares-pass.so");

    }

    // /laf

    if (aflcc->cmplog_mode) {

      insert_param(aflcc, "-fno-inline");

      load_llvm_pass(aflcc, "cmplog-switches-pass.so");
      // reuse split switches from laf
      load_llvm_pass(aflcc, "split-switches-pass.so");

    }

    // #if LLVM_MAJOR >= 13
    //     // Use the old pass manager in LLVM 14 which the AFL++ passes still
    //     use. insert_param(aflcc, "-flegacy-pass-manager");
    // #endif

    if (aflcc->lto_mode && !aflcc->have_c) {

      add_lto_linker(aflcc);
      add_lto_passes(aflcc);

    } else {

      if (aflcc->instrument_mode == INSTRUMENT_PCGUARD) {

        add_optimized_pcguard(aflcc);

      } else if (aflcc->instrument_mode == INSTRUMENT_LLVMNATIVE) {

        add_native_pcguard(aflcc);

      } else {

        load_llvm_pass(aflcc, "afl-llvm-pass.so");

      }

    }

    if (aflcc->cmplog_mode) {

      load_llvm_pass(aflcc, "cmplog-instructions-pass.so");
      load_llvm_pass(aflcc, "cmplog-routines-pass.so");

    }

    // insert_param(aflcc, "-Qunused-arguments");

  }

  process_params(aflcc, 0, argc, argv);

  add_misc_args(aflcc);

  add_sanitizers(aflcc, envp);

  add_no_builtin(aflcc);

  add_macro(aflcc);

  add_runtime(aflcc);

  insert_param(aflcc, NULL);

}

/* Main entry point */

int main(int argc, char **argv, char **envp) {

  aflcc_state_t *aflcc = malloc(sizeof(aflcc_state_t));
  aflcc_state_init(aflcc);

  init_callname(aflcc, argv[0]);

  if (getenv("AFL_DEBUG")) {

    aflcc->debug = 1;
    if (strcmp(getenv("AFL_DEBUG"), "0") == 0) unsetenv("AFL_DEBUG");

  } else if (getenv("AFL_QUIET"))

    be_quiet = 1;

  if ((getenv("AFL_PASSTHROUGH") || getenv("AFL_NOOPT")) && (!aflcc->debug))

    be_quiet = 1;

  check_environment_vars(envp);

  find_built_deps(aflcc);

  compiler_mode_by_callname(aflcc);
  compiler_mode_by_environ(aflcc);
  compiler_mode_by_cmdline(aflcc, argc, argv);

  instrument_mode_by_environ(aflcc);

  mode_final_checkout(aflcc, argc, argv);

  process_params(aflcc, 1, argc, argv);

  maybe_show_help(aflcc, argc, argv);

  mode_notification(aflcc);

  if (aflcc->debug) 
    debugf_args(argc, argv);

  edit_params(aflcc, argc, argv, envp);

  if (aflcc->debug) 
    debugf_args((s32)aflcc->cc_par_cnt, (char **)aflcc->cc_params);

  if (aflcc->passthrough) {

    argv[0] = aflcc->cc_params[0];
    execvp(aflcc->cc_params[0], (char **)argv);

  } else {

    execvp(aflcc->cc_params[0], (char **)aflcc->cc_params);

  }

  FATAL("Oops, failed to execute '%s' - check your PATH", 
          aflcc->cc_params[0]);

  return 0;

}

