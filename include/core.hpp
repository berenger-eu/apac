#pragma once
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
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

#define CHAR_ASSIGNEMENT '='
#define CHAR_INSTR_END ';'
#define CHAR_NEWLINE '\n'

struct const_arg
{
	bool is_const;
	bool is_ptr_or_ref;
	clang::CXXMethodDecl* method;
	clang::VarDecl *declaration;
	clang::FieldDecl *field;
	std::vector<const_arg *>dependencies;
};
typedef std::unordered_map<clang::Decl*, struct const_arg> TableConstArg;
typedef std::unordered_map<unsigned , clang::FileID> TableFileID;
typedef std::unordered_map<clang::Expr*,struct const_arg> TableConstArgExpr;

bool isPointerQualType(clang::QualType );
bool isReferenceQualType(clang::QualType );
clang::ValueDecl* getInnerPtr(clang::Expr*);
clang::ValueDecl* getInnerDecl(clang::Expr*);

bool valueInit(clang::VarDecl*);
