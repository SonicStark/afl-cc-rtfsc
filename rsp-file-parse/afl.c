#include "common.h"
#include "config.h"
#include "types.h"
#include "debug.h"
#include "alloc-inl.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>

/*************  MOCK TRICKS BEGIN *************/

#if !DETECT_LEAKS
const char* __asan_default_options() { return "detect_leaks=0"; }
#endif

#define MAX_PARAMS_NUM 2048
#define PARAM_MISS 0
#define parse_misc_params(...) PARAM_MISS
#define parse_fsanitize(...) PARAM_MISS
#define parse_linking_params(...) PARAM_MISS

typedef struct aflcc_state {

  u8 **cc_params;
  u32  cc_par_cnt;
  u8 debug;

} aflcc_state_t;

static inline void insert_param(aflcc_state_t *aflcc, u8 *param) {

  if (unlikely(aflcc->cc_par_cnt + 1 >= MAX_PARAMS_NUM))
    FATAL("Too many command line parameters, please increase MAX_PARAMS_NUM.");

  aflcc->cc_params[aflcc->cc_par_cnt++] = param;

}

static void process_params(aflcc_state_t *aflcc, u8 scan, u32 argc, char **argv);

int main(int argc, char **argv) {

  aflcc_state_t *aflcc = ck_alloc(sizeof(aflcc_state_t));
  aflcc->cc_params = ck_alloc(MAX_PARAMS_NUM * sizeof(u8 *));
  aflcc->cc_par_cnt = 1;
  aflcc->cc_params[0] = argv[0];
  aflcc->debug = 0;

  process_params(aflcc, 1, argc, argv);
  process_params(aflcc, 0, argc, argv);

  printf("===RESULT===" "\n");
  for (int i = 1; i < aflcc->cc_par_cnt; i++)
    printf("%s" "\n", aflcc->cc_params[i]);

  return 0;

}

/*************  MOCK TRICKS END *************/

/*************  COPY process_params AS-IS BEGIN *************/
static void process_params(aflcc_state_t *aflcc, u8 scan, u32 argc,
                           char **argv) {

  // for (u32 x = 0; x < argc; ++x) fprintf(stderr, "[%u] %s\n", x, argv[x]);

  /* Process the argument list. */

  u8 skip_next = 0;
  while (--argc) {

    u8 *cur = *(++argv);

    if (skip_next > 0) {

      skip_next--;
      continue;

    }

    if (PARAM_MISS != parse_misc_params(aflcc, cur, scan)) continue;

    if (PARAM_MISS != parse_fsanitize(aflcc, cur, scan)) continue;

    if (PARAM_MISS != parse_linking_params(aflcc, cur, scan, &skip_next, argv))
      continue;

    /* Response file support -----BEGIN-----
      We have two choices - move everything to the command line or
      rewrite the response files to temporary files and delete them
      afterwards. We choose the first for easiness.
      For clang, llvm::cl::ExpandResponseFiles does this, however it
      only has C++ interface. And for gcc there is expandargv in libiberty,
      written in C, but we can't simply copy-paste since its LGPL licensed.
      So here we use an equivalent FSM as alternative, and try to be compatible
      with the two above. See:
        - https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html
        - driver::expand_at_files in gcc.git/gcc/gcc.c
        - expandargv in gcc.git/libiberty/argv.c
        - llvm-project.git/clang/tools/driver/driver.cpp
        - ExpandResponseFiles in llvm-project.git/llvm/lib/Support/CommandLine.cpp
    */
    if (*cur == '@') {

      u8 *filename = cur + 1;
      if (aflcc->debug) { DEBUGF("response file=%s\n", filename); }

      // Check not found or empty? let the compiler complain if so.
      FILE *f = fopen(filename, "r");
      if (!f) {

        if (!scan) insert_param(aflcc, cur);
        continue;

      }

      struct stat st;
      if (fstat(fileno(f), &st) || !S_ISREG(st.st_mode) || st.st_size < 1) {

        fclose(f);
        if (!scan) insert_param(aflcc, cur);
        continue;

      }

      // Limit the number of response files, the max value
      // just keep consistent with expandargv. Only do this in
      // scan mode, and not touch rsp_count anymore in the next.
      static u32 rsp_count = 2000;
      if (scan) {

        if (rsp_count == 0) FATAL("Too many response files provided!");

        --rsp_count;

      }

      // argc, argv acquired from this rsp file. Note that
      // process_params ignores argv[0], we need to put a const "" here.
      u32    argc_read = 1;
      char **argv_read = ck_alloc(sizeof(char *));
      argv_read[0] = "";

      char *arg_buf = NULL;
      u64   arg_len = 0;

      enum fsm_state {

        fsm_whitespace,    // whitespace seen so far
        fsm_double_quote,  // have unpaired double quote
        fsm_single_quote,  // have unpaired single quote
        fsm_backslash,     // a backslash is seen with no unpaired quote
        fsm_normal         // a normal char is seen

      };

      // Workaround to append c to arg buffer, and append the buffer to argv
#define ARG_ALLOC(c)                                             \
  do {                                                           \
                                                                 \
    ++arg_len;                                                   \
    arg_buf = ck_realloc(arg_buf, (arg_len + 1) * sizeof(char)); \
    arg_buf[arg_len] = '\0';                                     \
    arg_buf[arg_len - 1] = (char)c;                              \
                                                                 \
  } while (0)

#define ARG_STORE()                                                \
  do {                                                             \
                                                                   \
    ++argc_read;                                                   \
    argv_read = ck_realloc(argv_read, argc_read * sizeof(char *)); \
    argv_read[argc_read - 1] = arg_buf;                            \
    arg_buf = NULL;                                                \
    arg_len = 0;                                                   \
                                                                   \
  } while (0)

      int cur_chr = (int)' ';  // init as whitespace, as a good start :)
      enum fsm_state state_ = fsm_whitespace;

      while (cur_chr != EOF) {

        switch (state_) {

          case fsm_whitespace:

            if (arg_buf) {

              ARG_STORE();
              break;

            }

            if (isspace(cur_chr)) {

              cur_chr = fgetc(f);

            } else if (cur_chr == (int)'\'') {

              state_ = fsm_single_quote;
              cur_chr = fgetc(f);

            } else if (cur_chr == (int)'"') {

              state_ = fsm_double_quote;
              cur_chr = fgetc(f);

            } else if (cur_chr == (int)'\\') {

              state_ = fsm_backslash;
              cur_chr = fgetc(f);

            } else {

              state_ = fsm_normal;

            }

            break;

          case fsm_normal:

            if (isspace(cur_chr)) {

              state_ = fsm_whitespace;

            } else if (cur_chr == (int)'\'') {

              state_ = fsm_single_quote;
              cur_chr = fgetc(f);

            } else if (cur_chr == (int)'\"') {

              state_ = fsm_double_quote;
              cur_chr = fgetc(f);

            } else if (cur_chr == (int)'\\') {

              state_ = fsm_backslash;
              cur_chr = fgetc(f);

            } else {

              ARG_ALLOC(cur_chr);
              cur_chr = fgetc(f);

            }

            break;

          case fsm_backslash:

            ARG_ALLOC(cur_chr);
            cur_chr = fgetc(f);
            state_ = fsm_normal;

            break;

          case fsm_single_quote:

            if (cur_chr == (int)'\\') {

              cur_chr = fgetc(f);
              if (cur_chr == EOF) break;
              ARG_ALLOC(cur_chr);

            } else if (cur_chr == (int)'\'') {

              state_ = fsm_normal;

            } else {

              ARG_ALLOC(cur_chr);

            }

            cur_chr = fgetc(f);
            break;

          case fsm_double_quote:

            if (cur_chr == (int)'\\') {

              cur_chr = fgetc(f);
              if (cur_chr == EOF) break;
              ARG_ALLOC(cur_chr);

            } else if (cur_chr == (int)'"') {

              state_ = fsm_normal;

            } else {

              ARG_ALLOC(cur_chr);

            }

            cur_chr = fgetc(f);
            break;

          default:
            break;

        }

      }

      if (arg_buf) { ARG_STORE(); }  // save the pending arg after EOF

#undef ARG_ALLOC
#undef ARG_STORE

      if (argc_read > 1) { process_params(aflcc, scan, argc_read, argv_read); }

      // We cannot free argv_read[] unless we don't need to keep any
      // reference in cc_params. Never free argv[0], the const "".
      if (scan) {

        while (argc_read > 1)
          ck_free(argv_read[--argc_read]);

        ck_free(argv_read);

      }

      continue;

    }                                /* Response file support -----END----- */

    if (!scan) insert_param(aflcc, cur);

  }

}
/*************  COPY process_params AS-IS END *************/
