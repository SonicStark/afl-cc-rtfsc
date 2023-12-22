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

void init_callname(aflcc_state_t *aflcc, u8 *argv0) {

  char *cname = NULL;

  if ((cname = strrchr(argv0, '/')) != NULL)
    cname++;
  else
    cname = argv0;

  aflcc->callname = cname;

  if (strlen(cname) > 2 &&
      (strncmp(cname + strlen(cname) - 2, "++", 2) == 0 ||
       strstr(cname, "-g++") != NULL))
    aflcc->plusplus_mode = 1;

}

/*
  in find_object() we look here:

  1. if obj_path is already set we look there first
  2. then we check the $AFL_PATH environment variable location if set
  3. next we check argv[0] if it has path information and use it
    a) we also check ../lib/afl
  4. if 3. failed we check /proc (only Linux, Android, NetBSD, DragonFly, and
     FreeBSD with procfs)
    a) and check here in ../lib/afl too
  5. we look into the AFL_PATH define (usually /usr/local/lib/afl)
  6. we finally try the current directory

  if all these attempts fail - we return NULL and the caller has to decide
  what to do.
*/

u8 *find_object(aflcc_state_t *aflcc, u8 *obj, u8 *argv0) {

  u8 *afl_path = getenv("AFL_PATH");
  u8 *slash = NULL, *tmp;

  if (afl_path) {

    tmp = alloc_printf("%s/%s", afl_path, obj);

    if (aflcc->debug) DEBUGF("Trying %s\n", tmp);

    if (!access(tmp, R_OK)) {

      aflcc->obj_path = afl_path;
      return tmp;

    }

    ck_free(tmp);

  }

  if (argv0) {

    slash = strrchr(argv0, '/');

    if (slash) {

      u8 *dir = ck_strdup(argv0);

      slash = strrchr(dir, '/');
      *slash = 0;

      tmp = alloc_printf("%s/%s", dir, obj);

      if (aflcc->debug) DEBUGF("Trying %s\n", tmp);

      if (!access(tmp, R_OK)) {

        aflcc->obj_path = dir;
        return tmp;

      }

      ck_free(tmp);
      tmp = alloc_printf("%s/../lib/afl/%s", dir, obj);

      if (aflcc->debug) DEBUGF("Trying %s\n", tmp);

      if (!access(tmp, R_OK)) {

        u8 *dir2 = alloc_printf("%s/../lib/afl", dir);
        aflcc->obj_path = dir2;
        ck_free(dir);
        return tmp;

      }

      ck_free(tmp);
      ck_free(dir);

    }

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__linux__) || \
    defined(__ANDROID__) || defined(__NetBSD__)
  #define HAS_PROC_FS 1
#endif
#ifdef HAS_PROC_FS
    else {

      char *procname = NULL;
  #if defined(__FreeBSD__) || defined(__DragonFly__)
      procname = "/proc/curproc/file";
  #elif defined(__linux__) || defined(__ANDROID__)
      procname = "/proc/self/exe";
  #elif defined(__NetBSD__)
      procname = "/proc/curproc/exe";
  #endif
      if (procname) {

        char    exepath[PATH_MAX];
        ssize_t exepath_len = readlink(procname, exepath, sizeof(exepath));
        if (exepath_len > 0 && exepath_len < PATH_MAX) {

          exepath[exepath_len] = 0;
          slash = strrchr(exepath, '/');

          if (slash) {

            *slash = 0;
            tmp = alloc_printf("%s/%s", exepath, obj);

            if (!access(tmp, R_OK)) {

              u8 *dir = alloc_printf("%s", exepath);
              aflcc->obj_path = dir;
              return tmp;

            }

            ck_free(tmp);
            tmp = alloc_printf("%s/../lib/afl/%s", exepath, obj);

            if (aflcc->debug) DEBUGF("Trying %s\n", tmp);

            if (!access(tmp, R_OK)) {

              u8 *dir = alloc_printf("%s/../lib/afl/", exepath);
              aflcc->obj_path = dir;
              return tmp;

            }

          }

        }

      }

    }

#endif
#undef HAS_PROC_FS

  }

  tmp = alloc_printf("%s/%s", AFL_PATH, obj);

  if (aflcc->debug) DEBUGF("Trying %s\n", tmp);

  if (!access(tmp, R_OK)) {

    aflcc->obj_path = AFL_PATH;
    return tmp;

  }

  ck_free(tmp);

  tmp = alloc_printf("./%s", obj);

  if (aflcc->debug) DEBUGF("Trying %s\n", tmp);

  if (!access(tmp, R_OK)) {

    aflcc->obj_path = ".";
    return tmp;

  }

  ck_free(tmp);

  if (aflcc->debug) DEBUGF("Trying ... giving up\n");

  return NULL;

}