#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTInitVisitor : public RecursiveASTVisitor<ASTInitVisitor> {
public:
  ASTInitVisitor(Rewriter &R, std::string &mainName,
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
  bool VisitCXXThisExpr(CXXThisExpr *);
  bool VisitCXXMethodDecl(CXXMethodDecl *);
  bool VisitFieldDecl(FieldDecl *);
  bool VisitVarDecl(VarDecl *);
  bool VisitCallExpr(CallExpr *);
  bool VisitBinaryOperator(BinaryOperator *);

private:
  Rewriter &TheRewriter;
  std::string &mainName;
  std::vector<std::string> &functions;
  std::vector<std::string> &functionsToIgnore;
  SymTab &SymT;
};
