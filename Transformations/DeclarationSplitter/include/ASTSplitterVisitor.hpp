#pragma once
#include <sstream>
#include <string>
#include <queue>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "common.hpp"

using namespace clang;
class ASTSplitterVisitor : public RecursiveASTVisitor<ASTSplitterVisitor>
{
public:
    ASTSplitterVisitor(Rewriter &R) : TheRewriter(R) {};
    inline bool VisitStmt(Stmt *) {return true;}
    bool VisitDeclStmt(DeclStmt*);

private:
    bool isValidSeparation(const VarDecl& );
    void stringVarDecl(const VarDecl&,std::stringstream& , std::stringstream& );
    Rewriter &TheRewriter;
};
