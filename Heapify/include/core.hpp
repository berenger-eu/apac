#pragma once
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"


#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"


using namespace clang;

struct item_found{
    std::string name;
    //unique, increments by 1 for each variable with the same name
    unsigned int uid;   
    clang::QualType qTypeTempMem;
    clang::QualType qTypeNew;
    clang::QualType qTypeVar;
    bool found;
    bool array;
    clang::VarDecl* declaration;
 };
extern struct item_found variableHeap;
extern struct item_found functionHeap;
extern std::unordered_map<std::string,int> varCounter;
extern std::vector<struct item_found> currentVarsEncountered; //TODO implement in cleaner manner
