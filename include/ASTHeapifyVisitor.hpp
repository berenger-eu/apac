#include "stringManipulation.hpp" 
#include "utilitaryFunctions.hpp"
using namespace clang;
class ASTHeapifyVisitor : public RecursiveASTVisitor<ASTHeapifyVisitor>
{
public:
    ASTHeapifyVisitor(Rewriter &R) : TheRewriter(R) {};
    inline bool VisitStmt(Stmt *) {return true;} 
    bool VisitFunctionDecl(FunctionDecl *); 
    bool VisitCompoundStmt(CompoundStmt *);
private:
    //Like Visit functions, but called by VisitCompoundStmt and not by default when encountering specific nodes
    std::string subVisitVarDecl(VarDecl& ,std::vector<struct item_found>&);
    void subVisitIfStmt(IfStmt*);
    void subVisitForStmt(ForStmt* );
    bool subVisitReturnStmt(ReturnStmt& );
    bool subVisitCompoundStmt(CompoundStmt* coSt);
    void handleSubStmt(Stmt* );
    void handleDeclStmt(DeclStmt*,std::vector<struct item_found>&);
    void initItem(struct item_found&,VarDecl&);
    Rewriter &TheRewriter;
};