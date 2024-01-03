# afl-cc-rtfsc

RTFSC of `afl-cc.c` in AFLplusplus :)

`afl-cc.c` has become bloated for a long time. I try to decompose it and understand its internal mechanisms during this process. A nice big monitor really helps me a lot.

The story starts from weird behaviors of `afl-clang-fast` which confused a friend of mine: running `afl-clang-fast` but actually same as `afl-gcc`. Not long after, I embarked on a long journey of exploring `afl-cc.c`...

## release v4.08c

This is the first try, based on [Version ++4.08c (release)](https://github.com/AFLplusplus/AFLplusplus/releases/tag/v4.08c).
Original source code locates in `release-4.08c-src` and the production locates in `release-4.08c-dat`.
It works well and successfully fixed the messes about `afl-clang-fast` and param parsing, but not elegant and compact enough.

## dev-5f492da7

Start to reorganize and forward all modifications to [PR 1912](https://github.com/AFLplusplus/AFLplusplus/pull/1912).
`dev-5f492da7-src` contains original source code copied as-is upon [head 5f492da](https://github.com/AFLplusplus/AFLplusplus/commit/5f492da71793e75e145e13d4c0dd9506bac2d60c).
The disassembled `afl-cc.c` is preserved in `dev-5f492da7-dat`, while the develops of AFL++ prefer to *keep all tools in a single file, with the exception of afl-fuzz*.

## tools

 - `apply-patch.sh`: Help to apply the disassembled `afl-cc.c` to AFL++ repo.
