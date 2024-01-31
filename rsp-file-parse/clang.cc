/* Based on clang/tools/driver/driver.cpp, the Clang GCC-Compatible Driver

  Git tag: llvmorg-12.0.0

  Compatible with LLVM versions >=11

  Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
  See https://llvm.org/LICENSE.txt for license information.
  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

*/

#include "clang/Driver/Driver.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetSelect.h"

using namespace clang;
using namespace clang::driver;
using namespace llvm::opt;

#if !DETECT_LEAKS
extern "C"
const char* __asan_default_options() { return "detect_leaks=0"; }
#endif

int main(int argc_, const char **argv_) {

  llvm::InitLLVM X(argc_, argv_);
  llvm::setBugReportMsg("PLEASE submit a bug report to "
                        "https://github.com/SonicStark/afl-cc-rtfsc"
                        " and include the crash backtrace.\n");
  SmallVector<const char *, 256> argv(argv_, argv_ + argc_);

  if (llvm::sys::Process::FixupStandardFileDescriptors())
    return 1;

  llvm::InitializeAllTargets();

  llvm::BumpPtrAllocator A;
  llvm::StringSaver Saver(A);

  // Parse response files using the GNU syntax. It just can't be used in CL mode :)
  bool MarkEOLs = false;

  llvm::cl::TokenizerCallback Tokenizer = &llvm::cl::TokenizeGNUCommandLine;

#if LLVM_VERSION_MAJOR >= 16
  llvm::cl::ExpansionContext ECtx(A, Tokenizer);
  ECtx.setMarkEOLs(MarkEOLs);
  if (llvm::Error Err = ECtx.expandResponseFiles(argv)) {
    llvm::errs() << toString(std::move(Err)) << '\n';
    return 1;
  }
#else
  llvm::cl::ExpandResponseFiles(Saver, Tokenizer, argv, MarkEOLs);
#endif

  llvm::outs() << "===RESULT===" "\n";

  for (int i = 1, size = argv.size(); i < size; ++i) {
    // Skip end-of-line response file markers
    if (argv[i] == nullptr)
      continue;
    llvm::outs() << argv[i] << "\n";
  }

  return 0;

}
