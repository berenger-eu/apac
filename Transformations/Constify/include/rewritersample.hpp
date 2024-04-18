#pragma once
#include "ASTPrintVisitor.hpp"
#include "ASTConstifyVisitor.hpp"
#include "ASTInitVisitor.hpp"
#include "SymTab.hpp"
#include <fstream>
#include <cstdlib>
#include <filesystem>

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Frontend/Rewriters.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"


#include "llvm/Support/Host.h"


#define PRINT_TABLE 1