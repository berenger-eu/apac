#pragma once
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <unordered_map>
#include <queue>

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
class ASTUnstackVisitor : public RecursiveASTVisitor<ASTUnstackVisitor>
{
public:
    ASTUnstackVisitor(Rewriter &R) : TheRewriter(R),tempVarsCounter(0),callsToIgnore(0) {};
    inline bool VisitStmt(Stmt *) {return true;} 
    bool VisitFunctionDecl(FunctionDecl*);
    bool VisitCompoundStmt(CompoundStmt*);
private:
    void handleSubStmt(Stmt* );
    void subVisitCallExpr(CallExpr* );
    void subVisitDeclStmt(DeclStmt* );
    void subVisitUnaryOperator(UnaryOperator* );
    void subVisitBinaryOperator(BinaryOperator* ); 
    void transformCallList(std::vector<CallExpr*>&,SourceLocation );
    void unstackTransformCallExpr(CallExpr*,SourceLocation);
    void recursiveStringCreateTransformCallExpr(CallExpr* ,std::vector<CallExpr*>& );
    std::string getExprAsString(Expr* );
    std::string createTempVarString(CallExpr* ,int ,std::queue<int>&);
    std::string createCallArgString(Expr* ,std::queue<int>& );

    void findTopCallsInExpr(Expr*,std::vector<CallExpr*>&);
    void transfoInstruction(std::vector<Expr*>&,Stmt*);
    void tranfoExpr(Expr* ,Stmt* );
    //Count the number of callExpr to ignore (because they are inside a call that has been parsed already)
    unsigned int callsToIgnore;
    //Used to name temp variables used to store the result of function calls
    int tempVarsCounter;  
    //Like Visit functions, but called by VisitCompoundStmt and not by default when encountering specific nodes
    Rewriter &TheRewriter;
};
