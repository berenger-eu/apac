#pragma once
#include <sstream>
#include <string>

#include "AliasTable.hpp"
#include "Instruction.hpp"
#include "InstructionsOrderManager.hpp"
#include "common.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;
class ASTTaskGraphVisitor : public RecursiveASTVisitor<ASTTaskGraphVisitor> {
public:
  ASTTaskGraphVisitor(Rewriter &R, StmtOrder &orderManager)
      : TheRewriter(R), orderManager(orderManager),
        currentOrderManager(&orderManager), aliasTable(R),
        ignoreStmtPragma(false) {};
  inline bool VisitStmt(Stmt *s) { return true; }
  // Traverse methods lets us stop visiting nodes that we don't need
  inline bool TraverseDeclStmt(DeclStmt *d) {
    if (isInHeaders(TheRewriter.getSourceMgr(), d->getBeginLoc()))
      return true;
    if (!ignoreStmtPragma)
      currentOrderManager->addInstructionToManager(d);
    return true;
  }
  inline bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *fDecl) {
    if (fDecl->getNameAsString().find("invalid_ref") == std::string::npos) {
      return RecursiveASTVisitor::TraverseFunctionTemplateDecl(fDecl);
    }
    return true;
  }

  inline bool TraverseGotoStmt(GotoStmt *g) {
    if (isInHeaders(TheRewriter.getSourceMgr(), g->getBeginLoc()))
      return true;
    if (!ignoreStmtPragma)
      currentOrderManager->addInstructionToManager(g);
    return true;
  }
  inline bool TraverseCXXMethodDecl(CXXMethodDecl *m) {
    return TraverseFunctionDecl(m);
  }
  bool TraverseFunctionDecl(FunctionDecl *f);
  // Calls respective handle method

  inline bool TraverseCXXMemberCallExpr(CXXMemberCallExpr *c) {
    return traverseSimpleElements(c);
  }
  inline bool TraverseCallExpr(CallExpr *c) {
    return traverseSimpleElements(c);
  }
  inline bool TraverseUnaryOperator(UnaryOperator *uop) {
    return traverseSimpleElements(uop);
  }
  inline bool TraverseBinaryOperator(BinaryOperator *bop) {
    return traverseSimpleElements(bop);
  }
  inline bool TraverseCompoundAssignOperator(CompoundAssignOperator *bop) {
    return traverseSimpleElements(bop);
  }
  inline bool TraverseCXXDeleteExpr(CXXDeleteExpr *d) {
    if (isInHeaders(TheRewriter.getSourceMgr(), d->getBeginLoc()))
      return true;
    if (!ignoreStmtPragma)
      currentOrderManager->addInstructionToManager(d);
    Instruction instr(d, getStmtAsString(d, TheRewriter.getLangOpts()));
    if (isa<DeclRefExpr>(d->getArgument()->IgnoreImpCasts())) {
      auto declRef = cast<DeclRefExpr>(d->getArgument()->IgnoreImpCasts());
      auto alias = aliasTable.getOrAddAliasArg(declRef);
      addDependencyWrite(instr, alias);
      addDependencyRead(instr, alias);
    }
    instr.noFusion = true;
    functionsInstructionsVector.back().push_back(instr);
    return true;
  }
  // Can be ignore if it is assumed that previous passes were run
  // So the return statement should not contain any expressions
  inline bool TraverseReturnStmt(ReturnStmt *r) {
    return true;
    // return traverseSimpleElements(r);
  }
  inline bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *c) {
    return traverseSimpleElements(c);
  }

  bool TraverseForStmt(ForStmt *f);
  bool TraverseIfStmt(IfStmt *i);
  const auto &getTaskGraphs() { return functionsInstructionsVector; }
  std::vector<std::vector<Instruction>> functionsInstructionsVector;

  const AliasTable &getAliasTable() const { return aliasTable; }
  inline void dumpInstructions() const {
    for (const auto &function : functionsInstructionsVector)
      for (const auto &instr : function)
        instr.dump();
  }

private:
  // Add all read dependencies of an expression to the instruction
  void addDependenciesRead(Instruction &instr, const Expr *e);

  inline void addDependencyRead(Instruction &instr,
                                std::shared_ptr<aliasArg> d) {
    for (auto &alias : aliasTable.getAliases(d)) {
      if (instr.dependencies.count(alias) == 0)
        instr.dependencies.insert({alias, NodeDependency{true, false}});
      else
        instr.dependencies.find(alias)->second.isRead = true;
    }
  }
  inline void addDependencyWrite(Instruction &instr,
                                 std::shared_ptr<aliasArg> d) {
    for (auto &alias : aliasTable.getAliases(d)) {
      if (instr.dependencies.count(alias) == 0)
        instr.dependencies.insert({alias, NodeDependency{false, true}});
      else
        instr.dependencies.find(alias)->second.isWrite = true;
    }
  }
  inline bool traverseSimpleElements(Stmt *s) {
    if (isInHeaders(TheRewriter.getSourceMgr(), s->getBeginLoc()))
      return true;
    if (!ignoreStmtPragma)
      currentOrderManager->addInstructionToManager(s);
    Instruction instr(s, getStmtAsString(s, TheRewriter.getLangOpts()));
    handleStmt(*s, instr);
    functionsInstructionsVector.back().push_back(instr);
    return true;
  }
  void handleCXXOperatorCallExpr(const CXXOperatorCallExpr &, Instruction &,
                                 bool isWrite = false);
  void handleUnaryOperator(const UnaryOperator &, Instruction &,
                           bool isWrite = false);
  void handleBinaryOperator(const BinaryOperator &, Instruction &,
                            bool isWrite = false);
  void handleCallExpr(const CallExpr &, Instruction &, bool isWrite = false);
  void handleStmt(const Stmt &st, Instruction &, bool isWrite = false,
                  bool isRead = true);
  void handleMemberCallExpr(const CXXMemberCallExpr &, Instruction &,
                            bool isWrite = false);
  void handleBinaryAssignment(const BinaryOperator &, Instruction &,
                              bool isWrite = false);
  void handlePointersBinaryAssignment(
      const BinaryOperator &, Instruction &, const VarDecl *, const Expr *,
      std::unordered_set<std::shared_ptr<aliasArg>> &);
  void computeAliasesForRHS(const Expr *bop,
                            std::unordered_set<std::shared_ptr<aliasArg>> &,
                            Instruction &instr);
  Rewriter &TheRewriter;
  StmtOrder &orderManager;
  StmtOrder *currentOrderManager;
  AliasTable aliasTable;
  bool ignoreStmtPragma;
};
inline bool isEmptyInstruction(const Instruction &instr) {
  return instr.dependencies.size() == 0;
};
inline bool isInExceptionList(const ParmVarDecl &p) {
  return p.getType().getAsString().find("std::shared_ptr") != std::string::npos;
}
// Get all the variables that are read in an expression (similar to
// getAllDeclRefExprInsideExpr but with some exceptions such as a in &a)
std::vector<const DeclRefExpr *> getAllReadDeclRefExprInsideExpr(const Expr *e);
