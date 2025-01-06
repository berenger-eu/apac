#pragma once
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
class ASTUnstackVisitor : public RecursiveASTVisitor<ASTUnstackVisitor> {
public:
  ASTUnstackVisitor(Rewriter &R)
      : TheRewriter(R), tempVarsCounter(0), callsToIgnore(0) {};
  inline bool VisitStmt(Stmt *) { return true; }
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (fDecl->getNameAsString().find("_apacSeq") == std::string::npos) {
      return RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  // Resets the counter for variables
  bool VisitFunctionDecl(FunctionDecl *);
  // Used so that we do not Visit callExpr twice
  //(first with any Visitor and then with the Visitor on CallExpr)
  bool VisitCompoundStmt(CompoundStmt *);

private:
  // Will call transfoInstruction for each type of Stmt with the necessary
  // arguments
  void handleSubStmt(Stmt *);
  // Will call transfoInstruction and give it the initialization of each
  // declared variable in DeclStmt
  void subVisitDeclStmt(DeclStmt *);
  // Will call transfoInstruction and give the call
  inline void subVisitCallExpr(CallExpr *calExpr) {
    std::stack<Expr *> exprList({calExpr});
    transfoInstruction(exprList, calExpr->getBeginLoc());
  }
  // Will call transfoInstruction and give the Expr on the Right side and the
  // Left side of the operator
  inline void subVisitBinaryOperator(BinaryOperator *bop) {
    std::stack<Expr *> exprList({bop->getLHS(), bop->getRHS()});
    transfoInstruction(exprList, bop->getBeginLoc());
  }
  // Will call transfoInstruction and give it the Expr linked to the Unary
  // operator
  inline void subVisitUnaryOperator(UnaryOperator *uop) {
    std::stack<Expr *> exprList({uop->getSubExpr()});
    transfoInstruction(exprList, uop->getBeginLoc());
  }

  // Replaces a given Call expression by __tempVar_X,
  // Will add all unstacked calls before the given location
  void unstackTransformCallExpr(CallExpr *, const SourceLocation &);

  // Recursive, will look for all CallExpr in the given CallExpr,
  //  will then add them all to the vector
  void findAllCallExpr(CallExpr *, std::vector<CallExpr *> &);

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

  // Find all CallExpr in the given Expr and adds them to the vector
  void findTopCallsInExpr(Expr *, std::vector<CallExpr *> &);
  // Transforms an instruction, which can be composed of multiple expressions
  // (BinaryOperator, multiple declaration) Will write its modified version at
  // the given SourceLocation
  void transfoInstruction(std::stack<Expr *> &,
                          const SourceLocation &instructionBeginLoc);
  // Transforms a single Expr
  // Will write its modified version at the given SourceLocation
  void tranfoExpr(Expr *, const SourceLocation &);
  // Like Visit functions, but called by VisitCompoundStmt and not by default
  // when encountering specific nodes
  Rewriter &TheRewriter;
  // Used to name temp variables used to store the result of function calls
  int tempVarsCounter;
  // Count the number of callExpr to ignore (because they are inside a call that
  // has been parsed already)
  unsigned int callsToIgnore;
};
