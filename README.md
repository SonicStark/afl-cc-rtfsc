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

## rsp-file-parse

New implementation for parsing response file (aka rsp-file) in `afl-cc`, harness it, fuzz it, and do some regression tests against `gcc` and `clang`. 

Recommanded prerequisites:
- AFLplusplus
- clang-16
- libclang-16-dev
- libclang-cpp16-dev
- libiberty-dev

Overview:
1. `afl.c`：Harness rsp-file parsing in `afl-cc`.
2. `gcc.c`：Harness rsp-file parsing in `gcc`.
3. `clang.cc`：Harness rsp-file parsing in `clang`.
4. `regression.py`：Read some inputs, send each of them to two specified harnesses (named Alpha and Bravo) of rsp-file parsing, compare the two results from Alpha and Bravo, and give report on this.
5. `build.sh`：Build harnesses mentioned above, as `afl-rsp`, `clg-rsp` and `gcc-rsp`. Also build `afl-rsp-fuzz` for fuzzing. The outputs would be located in `./build`. Please use clang for it!
6. `corpus`：Some rsp-files as initial seeds for fuzzing. Start fuzzing like:
   ```bash
   afl-fuzz -f /tmp/frsp -m none -i ./corpus -o ./fuzzout -- ./build/afl-rsp-fuzz @/tmp/frsp
   ```

Harmless inconsistence found so far:
1. `gcc` stop reading file when `\0` was seen, while `clang` keeps reading until `EOF` was seen and regards `\0` as a normal char. `afl-cc` follows the latter.
2. `clang` treats `\x0c` (aka `\f`) and `\x0b` (aka `\b`) as normal chars, while as spaces in `gcc` which is same as what `isspace(3)` does. `afl-cc` follows the latter.
3. `afl-cc` always suppresses spaces between two args, while `gcc` sometimes not.

## tools

 - `apply-patch.sh`: Help to apply the disassembled `afl-cc.c` to AFL++ repo.
