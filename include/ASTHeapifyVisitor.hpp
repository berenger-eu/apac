#include "stringManipulation.hpp" 
#include "utilitaryFunctions.hpp"
using namespace clang;
class ASTHeapifyVisitor : public RecursiveASTVisitor<ASTHeapifyVisitor>
{
public:
    ASTHeapifyVisitor(Rewriter &R) : TheRewriter(R) {};
    bool VisitStmt(Stmt *); 
    bool VisitFunctionDecl(FunctionDecl *); 
    bool VisitCompoundStmt(CompoundStmt *);
private:
    //Like Visit functions, but called by VisitCompoundStmt and not by default when encountering specific nodes
    std::string subVisitVarDecl(VarDecl& ,std::vector<item_found>&);
    bool subVisitReturnStmt(ReturnStmt& );
    bool subVisitCompoundStmt(CompoundStmt* coSt);

    Rewriter &TheRewriter;
};