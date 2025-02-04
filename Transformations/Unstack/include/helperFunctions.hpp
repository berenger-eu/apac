#pragma once
#include "clang/AST/ASTContext.h"

#include "common.hpp"

using namespace clang;
// Recursive, will look for all CallExpr in the given CallExpr,
//  will then add them all to the vector
void findAllCallExpr(CallExpr *, std::vector<CallExpr *> &);

// Find all CallExpr in the given Expr and adds them to the vector
void findTopCallsInExpr(Expr *, std::vector<CallExpr *> &);
