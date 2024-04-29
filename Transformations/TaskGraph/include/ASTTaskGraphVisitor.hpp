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
    bool VisitFunctionDecl(FunctionDecl *f);
    bool VisitVarDecl(VarDecl *v);
    bool VisitUnaryOperator(UnaryOperator* uop);
    bool VisitBinaryOperator(BinaryOperator* bop);
private:
    std::stack<PotTaskGraph> taskGraphs;
    Rewriter &TheRewriter;
};
