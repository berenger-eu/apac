#pragma once
#include <sstream>
#include <string>
#include <stack>

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

    const std::stack<PotTaskGraph>& getTaskGraphs(){return taskGraphs;}
private:
    bool isEmptyTask(const PotTask& task){return task.getParams().size()==0;};
    void handleUnaryOperator(const UnaryOperator& ,PotTask&);
    void handleBinaryOperator(const BinaryOperator& ,PotTask&);
    void handleCallExpr(const CallExpr& ,PotTask&);
    void handleExpr(const Expr& exp,PotTask&);
    std::stack<PotTaskGraph> taskGraphs;
    Rewriter &TheRewriter;
};
