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
	clang::VarDecl *declaration;
	std::vector<const_arg *>dependencies;
};
extern std::unordered_map<std::string, struct const_arg> const_arg_table;

const_arg* getHashTableValue (clang::NamedDecl* );
std::string getHashKey(clang::NamedDecl*);
void addDependencyHashTable(const_arg* ,const_arg*);
bool isPointerQualType(clang::QualType );
bool isReferenceQualType(clang::QualType );
clang::ValueDecl* getInnerPtr(clang::Expr*);
clang::ValueDecl* getInnerDecl(clang::Expr*);