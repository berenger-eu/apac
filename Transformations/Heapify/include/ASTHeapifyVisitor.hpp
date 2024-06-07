#include "stringManipulation.hpp" 
#include "utilitaryFunctions.hpp"
using namespace clang;
class ASTHeapifyVisitor : public RecursiveASTVisitor<ASTHeapifyVisitor>
{
public:
    ASTHeapifyVisitor(Rewriter &R,struct item_found& funHeap,struct item_found& varHeap)
     : TheRewriter(R),functionHeap(funHeap),variableHeap(varHeap) {};
    inline bool VisitStmt(Stmt *) {return true;} 
    bool VisitFunctionDecl(FunctionDecl *); 

private:
    //Like Visit functions, but called by VisitCompoundStmt and not by default when encountering specific nodes
    std::string subVisitVarDecl(VarDecl& ,std::vector<struct item_found>&);
    void subVisitIfStmt(IfStmt*);
    void subVisitForStmt(ForStmt* );
    void subVisitWhileStmt(WhileStmt* );
    bool deleteSegmentAtStmt(Stmt& st);
    bool subVisitCompoundStmt(CompoundStmt* coSt);
    void handleSubStmt(Stmt* );
    std::string stringDeclStmt(DeclStmt*,std::vector<struct item_found>&);
    void handleDeclStmt(DeclStmt*,std::vector<struct item_found>&);
    void initItem(struct item_found&,VarDecl&);
    void deleteSectionAfterCreatedScope(const SourceLocation& ,const std::vector<struct item_found>& );

    Rewriter &TheRewriter;
    struct item_found functionHeap;
    struct item_found variableHeap;
    std::unordered_map<std::string,int> varCounter;
    std::vector<struct item_found> currentVarsEncountered; //TODO implement in cleaner manner

};