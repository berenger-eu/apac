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

#include <vector>
#include <unordered_map>
 
 struct item_found{
    std::string name;
    //unique, increments by 1 for each variable with the same name
    unsigned int uid;   
    clang::QualType qTypeNew;
    clang::QualType qTypeVar;
    bool found;
    bool array;
    clang::VarDecl* declaration;
 };
std::unordered_map<std::string,int> varCounter;
std::vector<struct item_found> currentVarsInScope; //TODO implement in cleaner manner
 struct item_found variableHeap;
 struct item_found functionHeap;
using namespace clang;

bool isArrayVariable(VarDecl& );
//Creates a string to store the initialisation of a variable 
std::string createInitString(VarDecl& v);
//Creates a string for the deletion of a variable (delete ...)
std::string createDeleteString();
//Creates a segment containing all the delete strings
std::string createDeleteSegment();

//Creates a string for the creation of a variable (type* = new type)
std::string createCreationString(VarDecl& v);
bool foundCorrectFunction(Decl&);
bool foundCorrectVarType(VarDecl&);
bool foundCorrectVariable(VarDecl&);
bool isConstantInit(VarDecl& );
QualType unreferenceQType(QualType,const ASTContext& );
QualType referenceToQType(QualType,const ASTContext&);

class ASTHeapifyVisitor : public RecursiveASTVisitor<ASTHeapifyVisitor>
{
public:
    ASTHeapifyVisitor(Rewriter &R) : TheRewriter(R) {};
    bool VisitStmt(Stmt *); 
    bool VisitFunctionDecl(FunctionDecl *); 
    bool VisitCompoundStmt(CompoundStmt *);
private:
    //Like Visit functions, but called by VisitCompoundStmt and not by default when encountering specific nodes
    bool subVisitVarDecl(VarDecl& );
    bool subVisitReturnStmt(ReturnStmt& );
    Rewriter &TheRewriter;
};

