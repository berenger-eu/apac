#pragma once
#include "clang/AST/ASTContext.h"
#include <cstdio>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

#include "common.hpp"

#define CHAR_ASSIGNEMENT '='
#define CHAR_INSTR_END ';'
#define CHAR_NEWLINE '\n'

struct const_arg {
  bool is_const;
  bool is_ptr_or_ref;
  clang::CXXMethodDecl *method;
  clang::VarDecl *declaration;
  clang::FieldDecl *field;
  std::vector<const_arg *> dependencies;
};
typedef std::unordered_map<clang::Decl *, struct const_arg> TableConstArg;
typedef std::unordered_map<unsigned, clang::FileID> TableFileID;
typedef std::unordered_map<clang::Expr *, struct const_arg> TableConstArgExpr;

clang::Expr *getInnerPtr(clang::Expr *);
clang::Expr *getInnerExpr(clang::Expr *);

bool valueInit(clang::VarDecl *);
bool isExprACall(clang::Expr *expression);