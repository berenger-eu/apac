#include "ASTMultipleDeclSplitter.hpp"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include <iostream>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"

#include "clang/Rewrite/Frontend/Rewriters.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "llvm/Support/raw_ostream.h"

class MultipleDeclSplitterHandler {
public:
  static bool
  run(llvm::Expected<clang::tooling::CommonOptionsParser> &options,
      std::vector<std::string> &filesInput,
      const std::vector<std::string> &filesOutput = std::vector<std::string>());
};