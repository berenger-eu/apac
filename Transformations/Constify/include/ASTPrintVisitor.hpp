#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTPrintVisitor : public RecursiveASTVisitor<ASTPrintVisitor> {
public:
  ASTPrintVisitor(Rewriter &R, std::string &mainName,
                  std::vector<std::string> &functionsRef,
                  std::vector<std::string> &functionsToIgnoreRef,
                  SymTab &SymTableIn)
      : TheRewriter(R), mainName(mainName), functions(functionsRef),
        functionsToIgnore(functionsToIgnoreRef), SymT(SymTableIn) {}
  // To avoid errors on unused Stmt
  inline bool VisitStmt(Stmt *) { return true; };
  // bool VisitFunctionDecl(FunctionDecl *);
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (isToParseFunction(fDecl->getNameAsString(), functions,
                          functionsToIgnore, mainName)) {
      return RecursiveASTVisitor::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  bool VisitDeclStmt(DeclStmt *);
  bool VisitParmVarDecl(ParmVarDecl *);
  bool VisitCXXMethodDecl(CXXMethodDecl *);
  void PrepareRewriteVarDecl(VarDecl *, std::stringstream &);
  void rewriteSingleDecl(VarDecl *);
  void rewriteSingleDecl(ParmVarDecl *);

private:
  Rewriter &TheRewriter;
  std::string &mainName;
  std::vector<std::string> &functions;
  std::vector<std::string> &functionsToIgnore;
  SymTab &SymT;
};

void addConstToVar(ValueDecl *);
QualType addConstToQualType(QualType, ASTContext &);
QualType addConstToBuiltInType(QualType, ASTContext &);
QualType addConstToReferenceType(QualType, ASTContext &);
QualType addConstToPointerType(QualType, ASTContext &);