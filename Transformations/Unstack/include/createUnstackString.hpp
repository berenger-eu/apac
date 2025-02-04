#pragma once
#include "helperFunctions.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <queue>
#include <sstream>
#include <stack>
#include <string>

#include "common.hpp"
using namespace clang;
class UnstackTransformer {

public:
  UnstackTransformer(Rewriter &R) : TheRewriter(R) {};
  inline void transformFunctionsCalls(
      std::vector<std::map<SourceLocation, std::vector<CallExpr *>>>
          functionCalls) {
    for (auto function : functionCalls)
      transformFunctionCalls(function);
  }

private:
  // Replaces a given Call expression by __tempVar_X,
  // Will add all unstacked calls before the given location
  void unstackTransformCallExpr(CallExpr *, const SourceLocation &, int &);
  // Creates the string for an argument of a call,
  //  will replace calls within by the associated temporary variables
  //  This Expr : f(g(1)) will return the string : f(__temp_var_1)
  // The queue contains the ids to use in order, so the first element in the
  // previous example was 1
  std::string createCallArgString(Expr *, std::queue<int> &);
  // Creates the instruction for one of the temporary variable
  // String :  type __tempVar_x;
  //            __tempVar_x = unstackedCall;
  std::string createTempVarString(CallExpr *, int, std::queue<int> &);

  inline void transformFunctionCalls(
      std::map<SourceLocation, std::vector<CallExpr *>> callLocations) {
    int tempVarCounter = 0;
    for (auto locCallPair : callLocations) {
      for (auto call : locCallPair.second) {
        unstackTransformCallExpr(call, locCallPair.first, tempVarCounter);
      }
    }
  }
  Rewriter &TheRewriter;
};