#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTInitVisitor : public RecursiveASTVisitor<ASTInitVisitor> {
public:
  ASTInitVisitor(Rewriter &R, SymTab &SymTableIn)
      : TheRewriter(R), SymT(SymTableIn) {};
  inline bool VisitStmt(Stmt *) { return true; };
  bool VisitCXXThisExpr(CXXThisExpr *);
  bool VisitCXXMethodDecl(CXXMethodDecl *);
  bool VisitFieldDecl(FieldDecl *);
  bool VisitVarDecl(VarDecl *);
  bool VisitCallExpr(CallExpr *);
  bool VisitBinaryOperator(BinaryOperator *);

private:
  Rewriter &TheRewriter;
  SymTab &SymT;
};
