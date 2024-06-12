#pragma once
#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "PotTaskGraphInterface.hpp"
#include "common.hpp"
#include "AliasTable.hpp"

using namespace clang;
class ASTTaskGraphVisitor : public RecursiveASTVisitor<ASTTaskGraphVisitor>
{
public:
    ASTTaskGraphVisitor(Rewriter &R) : TheRewriter(R),aliasTable(R) {};
    inline bool VisitStmt(Stmt *s){return true;}
    //Traverse methods lets us stop visiting nodes that we don't need

    bool TraverseCXXMethodDecl(CXXMethodDecl *m);
    bool TraverseFunctionDecl(FunctionDecl *f);
    //Calls respective handle method

    bool TraverseCXXMemberCallExpr(CXXMemberCallExpr* c);
    bool TraverseCallExpr(CallExpr* c);
    bool TraverseUnaryOperator(UnaryOperator* uop);
    bool TraverseBinaryOperator(BinaryOperator* bop);
    bool TraverseCompoundAssignOperator(CompoundAssignOperator* bop);
    bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr* c);

    bool TraverseReturnStmt(ReturnStmt* r);
    bool TraverseForStmt(ForStmt* f);
    bool TraverseIfStmt(IfStmt* i);
    const auto& getTaskGraphs(){return functionsInstructionsVector;}
    std::vector<std::vector<Instruction>> functionsInstructionsVector;
private:
    bool isEmptyInstruction(const Instruction& instr){return instr.dependencies.size()==0;};
    void addDependency(Instruction& instr,Access a,const VarDecl* d);

    void handleUnaryOperator(const UnaryOperator& ,Instruction&);
    void handleBinaryOperator(const BinaryOperator& ,Instruction&);
    void handleCallExpr(const CallExpr& ,Instruction&);
    void handleExpr(const Expr& exp,Instruction&);
    void handleMemberCallExpr(const CXXMemberCallExpr& ,Instruction&);
    void computeAliasesForRHS(const BinaryOperator& bop,std::unordered_set<const VarDecl*>&, Instruction& instr);
    Rewriter &TheRewriter;
    AliasTable aliasTable;
};

bool isInExceptionList(const ParmVarDecl& p);