#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTInitVisitor : public APACRecursiveASTVisitor<ASTInitVisitor> {
public:
  ASTInitVisitor(Rewriter &R, std::string &mainName,
                 std::vector<std::string> &functionsRef,
                 std::vector<std::string> &functionsToIgnoreRef,
                 SymTab &SymTableIn)
      : APACRecursiveASTVisitor(R, mainName, functionsRef,
                                functionsToIgnoreRef),
        SymT(SymTableIn) {}

  bool VisitCXXThisExpr(CXXThisExpr *);
  bool VisitCXXMethodDecl(CXXMethodDecl *);
  bool VisitFieldDecl(FieldDecl *);
  bool VisitVarDecl(VarDecl *);
  bool VisitCallExpr(CallExpr *);
  bool VisitBinaryOperator(BinaryOperator *);

private:
  SymTab &SymT;
};
