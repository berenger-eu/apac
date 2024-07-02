#pragma once
#include "ASTConstifyVisitor.hpp"
#include "ASTInitVisitor.hpp"
#include "ASTPrintVisitor.hpp"
#include "SymTab.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Frontend/Rewriters.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#define PRINT_TABLE 1