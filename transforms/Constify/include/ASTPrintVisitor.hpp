#include "SymTab.hpp"
#include "core.hpp"
using namespace clang;
class ASTPrintVisitor : public APACRecursiveASTVisitor<ASTPrintVisitor> {
public:
  ASTPrintVisitor(Rewriter &R, std::string &mainName,
                  std::vector<std::string> &functionsRef,
                  std::vector<std::string> &functionsToIgnoreRef,
                  SymTab &SymTableIn)
      : APACRecursiveASTVisitor(R, mainName, functionsRef,
                                functionsToIgnoreRef),
        SymT(SymTableIn) {}

  bool VisitDeclStmt(DeclStmt *);
  bool VisitParmVarDecl(ParmVarDecl *);
  bool VisitCXXMethodDecl(CXXMethodDecl *);
  void PrepareRewriteVarDecl(VarDecl *, std::stringstream &);
  void rewriteSingleDecl(VarDecl *);
  void rewriteSingleDecl(ParmVarDecl *);

private:
  SymTab &SymT;
};

void addConstToVar(ValueDecl *);
QualType addConstToQualType(QualType, ASTContext &);
QualType addConstToBuiltInType(QualType, ASTContext &);
QualType addConstToReferenceType(QualType, ASTContext &);
QualType addConstToPointerType(QualType, ASTContext &);