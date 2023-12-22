#include "afl-cc.h"

static void help_env_vars(aflcc_state_t *aflcc) {

  SAYF(
      "Environment variables used:\n"
      "  AFL_CC: path to the C compiler to use\n"
      "  AFL_CXX: path to the C++ compiler to use\n"
      "  AFL_DEBUG: enable developer debugging output\n"
      "  AFL_DONT_OPTIMIZE: disable optimization instead of -O3\n"
      "  AFL_NO_BUILTIN: no builtins for string compare functions (for "
      "libtokencap.so)\n"
      "  AFL_NOOP: behave like a normal compiler (to pass configure "
      "tests)\n"
      "  AFL_PATH: path to instrumenting pass and runtime  "
      "(afl-compiler-rt.*o)\n"
      "  AFL_IGNORE_UNKNOWN_ENVS: don't warn on unknown env vars\n"
      "  AFL_INST_RATIO: percentage of branches to instrument\n"
      "  AFL_QUIET: suppress verbose output\n"
      "  AFL_HARDEN: adds code hardening to catch memory bugs\n"
      "  AFL_USE_ASAN: activate address sanitizer\n"
      "  AFL_USE_CFISAN: activate control flow sanitizer\n"
      "  AFL_USE_MSAN: activate memory sanitizer\n"
      "  AFL_USE_UBSAN: activate undefined behaviour sanitizer\n"
      "  AFL_USE_TSAN: activate thread sanitizer\n"
      "  AFL_USE_LSAN: activate leak-checker sanitizer\n");

  if (aflcc->have_gcc_plugin)
    SAYF(
        "\nGCC Plugin-specific environment variables:\n"
        "  AFL_GCC_CMPLOG: log operands of comparisons (RedQueen mutator)\n"
        "  AFL_GCC_OUT_OF_LINE: disable inlined instrumentation\n"
        "  AFL_GCC_SKIP_NEVERZERO: do not skip zero on trace counters\n"
        "  AFL_GCC_INSTRUMENT_FILE: enable selective instrumentation by "
        "filename\n");

#if LLVM_MAJOR >= 9
  #define COUNTER_BEHAVIOUR \
    "  AFL_LLVM_SKIP_NEVERZERO: do not skip zero on trace counters\n"
#else
  #define COUNTER_BEHAVIOUR \
    "  AFL_LLVM_NOT_ZERO: use cycling trace counters that skip zero\n"
#endif
  if (aflcc->have_llvm)
    SAYF(
        "\nLLVM/LTO/afl-clang-fast/afl-clang-lto specific environment "
        "variables:\n"
        "  AFL_LLVM_THREADSAFE_INST: instrument with thread safe counters, "
        "disables neverzero\n"

        COUNTER_BEHAVIOUR

        "  AFL_LLVM_DICT2FILE: generate an afl dictionary based on found "
        "comparisons\n"
        "  AFL_LLVM_DICT2FILE_NO_MAIN: skip parsing main() for the "
        "dictionary\n"
        "  AFL_LLVM_LAF_ALL: enables all LAF splits/transforms\n"
        "  AFL_LLVM_LAF_SPLIT_COMPARES: enable cascaded comparisons\n"
        "  AFL_LLVM_LAF_SPLIT_COMPARES_BITW: size limit (default 8)\n"
        "  AFL_LLVM_LAF_SPLIT_SWITCHES: cascaded comparisons on switches\n"
        "  AFL_LLVM_LAF_SPLIT_FLOATS: cascaded comparisons on floats\n"
        "  AFL_LLVM_LAF_TRANSFORM_COMPARES: cascade comparisons for string "
        "functions\n"
        "  AFL_LLVM_ALLOWLIST/AFL_LLVM_DENYLIST: enable "
        "instrument allow/\n"
        "    deny listing (selective instrumentation)\n");

  if (aflcc->have_llvm)
    SAYF(
        "  AFL_LLVM_CMPLOG: log operands of comparisons (RedQueen "
        "mutator)\n"
        "  AFL_LLVM_INSTRUMENT: set instrumentation mode:\n"
        "    CLASSIC, PCGUARD, LTO, GCC, CLANG, CALLER, CTX, NGRAM-2 "
        "..-16\n"
        " You can also use the old environment variables instead:\n"
        "  AFL_LLVM_USE_TRACE_PC: use LLVM trace-pc-guard instrumentation\n"
        "  AFL_LLVM_CALLER: use single context sensitive coverage (for "
        "CLASSIC)\n"
        "  AFL_LLVM_CTX: use full context sensitive coverage (for "
        "CLASSIC)\n"
        "  AFL_LLVM_NGRAM_SIZE: use ngram prev_loc count coverage (for "
        "CLASSIC)\n");

#ifdef AFL_CLANG_FLTO
  if (aflcc->have_lto)
    SAYF(
        "\nLTO/afl-clang-lto specific environment variables:\n"
        "  AFL_LLVM_MAP_ADDR: use a fixed coverage map address (speed), "
        "e.g. "
        "0x10000\n"
        "  AFL_LLVM_DOCUMENT_IDS: write all edge IDs and the corresponding "
        "functions\n"
        "    into this file\n"
        "  AFL_LLVM_LTO_DONTWRITEID: don't write the highest ID used to a "
        "global var\n"
        "  AFL_LLVM_LTO_STARTID: from which ID to start counting from for "
        "a "
        "bb\n"
        "  AFL_REAL_LD: use this lld linker instead of the compiled in "
        "path\n"
        "  AFL_LLVM_LTO_SKIPINIT: don't inject initialization code "
        "(used in WAFL mode)\n"
        "If anything fails - be sure to read README.lto.md!\n");
#endif

  SAYF(
      "\nYou can supply --afl-noopt to not instrument, like AFL_NOOPT. "
      "(this is helpful\n"
      "in some build systems if you do not want to instrument "
      "everything.\n");

}


void maybe_show_help(aflcc_state_t *aflcc, int argc, char **argv) {

  if (argc < 2 || strncmp(argv[1], "-h", 2) == 0) {

    printf("afl-cc" VERSION
           " by Michal Zalewski, Laszlo Szekeres, Marc Heuse\n");

    SAYF(
        "\n"
        "afl-cc/afl-c++ [options]\n"
        "\n"
        "This is a helper application for afl-fuzz. It serves as a drop-in "
        "replacement\n"
        "for gcc and clang, letting you recompile third-party code with the "
        "required\n"
        "runtime instrumentation. A common use pattern would be one of the "
        "following:\n\n"

        "  CC=afl-cc CXX=afl-c++ ./configure --disable-shared\n"
        "  cmake -DCMAKE_C_COMPILERC=afl-cc -DCMAKE_CXX_COMPILER=afl-c++ .\n"
        "  CC=afl-cc CXX=afl-c++ meson\n\n");

    SAYF(
        "                                       |------------- FEATURES "
        "-------------|\n"
        "MODES:                                  NCC PERSIST DICT   LAF "
        "CMPLOG SELECT\n"
        "  [LLVM] LLVM:             %s%s\n"
        "      PCGUARD              %s      yes yes     module yes yes    "
        "yes\n"
        "      CLASSIC              %s      no  yes     module yes yes    "
        "yes\n"
        "        - NORMAL\n"
        "        - CALLER\n"
        "        - CTX\n"
        "        - NGRAM-{2-16}\n"
        "  [LTO] LLVM LTO:          %s%s\n"
        "      PCGUARD              DEFAULT      yes yes     yes    yes yes "
        "   yes\n"
        "      CLASSIC                           yes yes     yes    yes yes "
        "   yes\n"
        "  [GCC_PLUGIN] gcc plugin: %s%s\n"
        "      CLASSIC              DEFAULT      no  yes     no     no  no     "
        "yes\n"
        "  [GCC/CLANG] simple gcc/clang: %s%s\n"
        "      CLASSIC              DEFAULT      no  no      no     no  no     "
        "no\n\n",
        aflcc->have_llvm ? "AVAILABLE" : "unavailable!",
        aflcc->compiler_mode == LLVM ? " [SELECTED]" : "",
        aflcc->have_lto ? "AVAILABLE" : "unavailable!",
        aflcc->compiler_mode == LTO ? " [SELECTED]" : "",
        LLVM_MAJOR >= 7 ? "DEFAULT" : "       ",
        LLVM_MAJOR >= 7 ? "       " : "DEFAULT",
        aflcc->have_gcc_plugin ? "AVAILABLE" : "unavailable!",
        aflcc->compiler_mode == GCC_PLUGIN ? " [SELECTED]" : "",
        aflcc->have_gcc ? "AVAILABLE" : "unavailable!",
        (aflcc->compiler_mode == GCC || aflcc->compiler_mode == CLANG) ? " [SELECTED]" : "");

    SAYF(
        "Modes:\n"
        "  To select the compiler mode use a symlink version (e.g. "
        "afl-clang-fast), set\n"
        "  the environment variable AFL_CC_COMPILER to a mode (e.g. LLVM) or "
        "use the\n"
        "  command line parameter --afl-MODE (e.g. --afl-llvm). If none is "
        "selected,\n"
        "  afl-cc will select the best available (LLVM -> GCC_PLUGIN -> GCC).\n"
        "  The best is LTO but it often needs RANLIB and AR settings outside "
        "of afl-cc.\n\n");

#if LLVM_MAJOR > 10 || (LLVM_MAJOR == 10 && LLVM_MINOR > 0)
  #define NATIVE_MSG                                                   \
    "  LLVM-NATIVE:  use llvm's native PCGUARD instrumentation (less " \
    "performant)\n"
#else
  #define NATIVE_MSG ""
#endif

    SAYF(
        "Sub-Modes: (set via env AFL_LLVM_INSTRUMENT, afl-cc selects the best "
        "available)\n"
        "  PCGUARD: Dominator tree instrumentation (best!) (README.llvm.md)\n"

        NATIVE_MSG

        "  CLASSIC: decision target instrumentation (README.llvm.md)\n"
        "  CALLER:  CLASSIC + single callee context "
        "(instrumentation/README.ctx.md)\n"
        "  CTX:     CLASSIC + full callee context "
        "(instrumentation/README.ctx.md)\n"
        "  NGRAM-x: CLASSIC + previous path "
        "((instrumentation/README.ngram.md)\n\n");

#undef NATIVE_MSG

    SAYF(
        "Features: (see documentation links)\n"
        "  NCC:    non-colliding coverage [automatic] (that is an amazing "
        "thing!)\n"
        "          (instrumentation/README.lto.md)\n"
        "  PERSIST: persistent mode support [code] (huge speed increase!)\n"
        "          (instrumentation/README.persistent_mode.md)\n"
        "  DICT:   dictionary in the target [yes=automatic or LLVM module "
        "pass]\n"
        "          (instrumentation/README.lto.md + "
        "instrumentation/README.llvm.md)\n"
        "  LAF:    comparison splitting [env] "
        "(instrumentation/README.laf-intel.md)\n"
        "  CMPLOG: input2state exploration [env] "
        "(instrumentation/README.cmplog.md)\n"
        "  SELECT: selective instrumentation (allow/deny) on filename or "
        "function [env]\n"
        "          (instrumentation/README.instrument_list.md)\n\n");

    if (argc < 2 || strncmp(argv[1], "-hh", 3)) {

      SAYF(
          "To see all environment variables for the configuration of afl-cc "
          "use \"-hh\".\n");

    } else {

      help_env_vars(aflcc);

    }

    SAYF(
        "\nFor any information on the available instrumentations and options "
        "please \n"
        "consult the README.md, especially section 3.1 about instrumenting "
        "targets.\n\n");

#if (LLVM_MAJOR >= 3)
    if (aflcc->have_lto)
      SAYF("afl-cc LTO with ld=%s %s\n", AFL_REAL_LD, AFL_CLANG_FLTO);
    if (aflcc->have_llvm)
      SAYF("afl-cc LLVM version %d using the binary path \"%s\".\n", LLVM_MAJOR,
           LLVM_BINDIR);
#endif

#ifdef USEMMAP
  #if !defined(__HAIKU__)
    SAYF("Compiled with shm_open support.\n");
  #else
    SAYF("Compiled with shm_open support (adds -lrt when linking).\n");
  #endif
#else
    SAYF("Compiled with shmat support.\n");
#endif
    SAYF("\n");

    SAYF(
        "Do not be overwhelmed :) afl-cc uses good defaults if no options are "
        "selected.\n"
        "Read the documentation for FEATURES though, all are good but few are "
        "defaults.\n"
        "Recommended is afl-clang-lto with AFL_LLVM_CMPLOG or afl-clang-fast "
        "with\n"
        "AFL_LLVM_CMPLOG and "
        "AFL_LLVM_DICT2FILE+AFL_LLVM_DICT2FILE_NO_MAIN.\n\n");

    if (LLVM_MAJOR < 13) {

      SAYF(
          "Warning: It is highly recommended to use at least LLVM version 13 "
          "(or better, higher) rather than %d!\n\n",
          LLVM_MAJOR);

    }

    exit(1);

  }

}
