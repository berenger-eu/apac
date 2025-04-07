#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTConstifyVisitor : public RecursiveASTVisitor<ASTConstifyVisitor> {
public:
  ASTConstifyVisitor(Rewriter &R, std::string &mainName,
                     std::vector<std::string> &functionsRef,
                     std::vector<std::string> &functionsToIgnoreRef,
                     SymTab &SymTableIn)
      : TheRewriter(R), mainName(mainName), functions(functionsRef),
        functionsToIgnore(functionsToIgnoreRef), SymT(SymTableIn) {};

  inline bool VisitStmt(Stmt *) { return true; };
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (isToParseFunction(fDecl->getNameAsString(), functions,
                          functionsToIgnore, mainName)) {
      return RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  bool VisitCXXMethodDecl(CXXMethodDecl *);
  bool VisitBinaryOperator(BinaryOperator *);
  bool VisitUnaryOperator(UnaryOperator *);
  bool VisitReturnStmt(ReturnStmt *);
  // bool VisitCXXMemberCallExpr(CXXMemberCallExpr* );
  bool VisitCallExpr(CallExpr *);
  bool VisitVarDecl(VarDecl *);

private:
  Rewriter &TheRewriter;
  std::string &mainName;
  std::vector<std::string> &functions;
  std::vector<std::string> &functionsToIgnore;
  SymTab &SymT;
};
void unconstifyByPropagation(const_arg *);
