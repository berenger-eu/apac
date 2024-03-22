#pragma once
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"


#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
class ASTGotoVisitor : public RecursiveASTVisitor<ASTGotoVisitor>
{
public:
    ASTGotoVisitor(Rewriter &R) : TheRewriter(R),functionsCounter(0) {};
    inline bool VisitStmt(Stmt *) {return true;} 
    bool VisitFunctionDecl(FunctionDecl*); 
private:
    void subVisitIfStmt(IfStmt* );
    void subVisitCompoundStmt(CompoundStmt* );
    void subVisitWhileStmt(WhileStmt* );
    void subVisitForStmt(ForStmt* );
    void subVisitReturnStmt(ReturnStmt* );
    void handleSubStmt(Stmt* );
    //Used to give a unique number for the exit section of each function
    unsigned int functionsCounter;  
    //Like Visit functions, but called by VisitCompoundStmt and not by default when encountering specific nodes
    Rewriter &TheRewriter;
};

std::string createGotoString(ReturnStmt& ,Rewriter& ,unsigned int&);
