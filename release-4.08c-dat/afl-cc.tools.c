#include "afl-cc.h"

static u8 cwd[4096];

char instrument_mode_string[18][18] = {

    "DEFAULT",
    "CLASSIC",
    "PCGUARD",
    "CFG",
    "LTO",
    "PCGUARD-NATIVE",
    "GCC",
    "CLANG",
    "CTX",
    "CALLER",
    "",
    "",
    "",
    "",
    "",
    "",
    "NGRAM",
    ""

};

u8 *instrument_mode_2str(instrument_mode_id i) {
  return instrument_mode_string[i];
}

char compiler_mode_string[7][12] = {

    "AUTOSELECT", "LLVM-LTO", "LLVM", "GCC_PLUGIN",
    "GCC",        "CLANG",    ""

};

u8 *compiler_mode_2str(compiler_mode_id i) {
  return compiler_mode_string[i];
}

void aflcc_state_init(aflcc_state_t *aflcc) {
  
  // Default NULL/0 is a good start
  memset(aflcc, 0, sizeof(aflcc_state_t));

  aflcc->cc_par_cnt = 1;
  aflcc->lto_flag = AFL_CLANG_FLTO;
  
  // aflcc->march_opt = CFLAGS_OPT;

}

u8 *getthecwd() {

  if (getcwd(cwd, sizeof(cwd)) == NULL) {

    static u8 fail[] = "";
    return fail;

  }

  return cwd;

}
