#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

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

 
 struct item_found{
    std::string name;
    bool found;
    bool array;
 };

 struct item_found variableHeap;
 struct item_found functionHeap;
using namespace clang;

 bool isArrayVariable(VarDecl* );
 std::string getInitString(VarDecl* v);
 std::string getDeleteString();
 std::string getCreationString(VarDecl* v);


class ASTHeapifyVisitor : public RecursiveASTVisitor<ASTHeapifyVisitor>
{
public:
    ASTHeapifyVisitor(Rewriter &R) : TheRewriter(R) {};
    bool VisitStmt(Stmt *);
    bool VisitFunctionDecl(FunctionDecl *);
    bool VisitCompoundStmt(CompoundStmt *);
    bool subVisitVarDecl(VarDecl* );
    bool subVisitReturnStmt(ReturnStmt* );
private:
    Rewriter &TheRewriter;
};

