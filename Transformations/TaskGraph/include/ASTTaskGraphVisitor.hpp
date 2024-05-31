#pragma once
#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "PotTaskGraphInterface.hpp"
#include "common.hpp"

using namespace clang;
class ASTTaskGraphVisitor : public RecursiveASTVisitor<ASTTaskGraphVisitor>
{
public:
    ASTTaskGraphVisitor(Rewriter &R) : TheRewriter(R) {};
    inline bool VisitStmt(Stmt *s){return true;}
    //Traverse methods lets us stop visiting nodes that we don't need


    bool TraverseFunctionDecl(FunctionDecl *f);
    //Calls respective handle method
    bool TraverseCallExpr(CallExpr* c);
    bool TraverseUnaryOperator(UnaryOperator* uop);
    bool TraverseBinaryOperator(BinaryOperator* bop);
    bool TraverseCompoundAssignOperator(CompoundAssignOperator* bop);

    bool TraverseReturnStmt(ReturnStmt* r);
    bool TraverseForStmt(ForStmt* f);
    bool TraverseIfStmt(IfStmt* i);
    const auto& getTaskGraphs(){return functionsInstructionsVector;}
    std::vector<std::vector<Instruction>> functionsInstructionsVector;
private:
    bool isEmptyInstruction(const Instruction& instr){return instr.dependencies.size()==0;};
    void handleUnaryOperator(const UnaryOperator& ,Instruction&);
    void handleBinaryOperator(const BinaryOperator& ,Instruction&);
    void handleCallExpr(const CallExpr& ,Instruction&);
    void handleExpr(const Expr& exp,Instruction&);

    Rewriter &TheRewriter;
};
