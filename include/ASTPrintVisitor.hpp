#include "core.hpp"
extern std::unordered_map<std::string, struct const_arg> const_arg_table;
using namespace clang;
class ASTPrintVisitor : public RecursiveASTVisitor<ASTPrintVisitor>
{
public:
    ASTPrintVisitor(Rewriter &R) : TheRewriter(R) {}
    // To avoid errors on unused Stmt
    bool VisitStmt(Stmt *);
    //bool VisitFunctionDecl(FunctionDecl *);
    bool VisitDeclStmt(DeclStmt* );
    bool VisitParmVarDecl(ParmVarDecl*);
    void PrepareRewriteVarDecl(VarDecl *,std::stringstream&);
    void rewriteSingleDecl(VarDecl* );

private:
   Rewriter &TheRewriter;
};

void addConstToVar(ValueDecl*);
void addConstToBuiltInType(ValueDecl*);
void addConstToReference(ValueDecl*);
void addConstToPointer(ValueDecl*);