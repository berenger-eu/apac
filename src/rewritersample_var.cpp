//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <iostream>

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
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
/*
class MyASTVisitorConstify : public RecursiveASTVisitor<MyASTVisitorConstify> {
	public:
		MyASTVisitorConstify(Rewriter &R):TheRewriter(R) {}
	bool VisitVarDecl(VarDecl* v)
	{
		

		return true;
	}
	private:
		Rewriter &TheRewriter;
};
*/
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R){}
/*
  bool VisitFuncDecl (FunctionDecl* fd){
		TheRewriter.InsertText(fd->getBeginLoc(),"jzfzé\ni",true,true);
		  return true;
		  }
  */
  
  bool VisitParmVarDecl(ParmVarDecl *pv) {
//	pv->setType(pv->getType().withConst());
//	TheRewriter.RemoveText(SourceRange(pv->getTypeSpecStartLoc(),pv->getTypeSpecEndLoc()));
//	TheRewriter.InsertText(pv->getBeginLoc(),"test",true,true);
    
    return true;
  }

 /*
  bool VisitCXXRecordDecl(CXXRecordDecl* rd)
  {	for(const FieldDecl& field: rd->fields){	
		TheRewriter.RemoveText(SourceRange(field.getBeginLoc(),field.getTypeSpecEndLoc()));
		TheRewriter.InsertText(field.getBeginLoc(),field.getType().getAsString(),false,true);
	}
 
	  return true;
  }
  */
  /*
  bool VisitFieldDecl(FieldDecl* fd){
	  TheRewriter.RemoveText(SourceRange(fd->getBeginLoc(),fd->getTypeSpecStartLoc()));
	  fd->setType(fd->getType().withConst());
	  TheRewriter.InsertText(fd->getBeginLoc(),fd->getType().getAsString(),false,true);
	  return true;
  }*/
 
  /*bool VisitDeclStmt(DeclStmt *declStatement){
	//Decl* decl;
	if((declStatement->isSingleDecl()));
	else
	{

		DeclGroup& dgr=declStatement->getDeclGroup().getDeclGroup();
		std::stringstream SSnotConst;
		std::stringstream SSconst;
		//SSnotConst<<"Not Const S\n";
		
		//SSconst<<"Const S\n";
		std::string typeString;
		VarDecl *v=cast<VarDecl>(dgr[0]);
		QualType qtype=v->getType();
		typeString=qtype.getAsString();
		v->setType(v->getType().withConst());
		for(int i=0;i<dgr.size();i++)
		{
			
			v=cast<VarDecl>(dgr[i]);
			qtype=v->getType();
			std::stringstream* SScurrent;
			if (qtype.isConstQualified())
				SScurrent=&SSconst;
			else
				SScurrent=&SSnotConst;
		Expr* expres;
		std::stringstream value;
		value<<"";
		if((expres =v->getInit()))
		{
		LangOptions lopt=TheRewriter.getLangOpts();
		SourceManager& sm=TheRewriter.getSourceMgr();
		SourceRange sr=expres->getSourceRange();//SourceRange(v->getBeginLoc(),v->getEndLoc());
		CharSourceRange chsr=Lexer::getAsCharRange(sr,sm,lopt);
		chsr=Lexer::makeFileCharRange(chsr,sm,lopt);
		value<<'='<<Lexer::Stringify(Lexer::getSourceText(chsr,sm,lopt));
		}
		//		std::stringstream value;
		//	uint64_t size;
		//	v->getInit()->printPretty(value,0);
		//	v->getInit()->evaluateCharRangeAsString(value,&size,NULL,v->getASTContext(),NULL);
			*SScurrent<<v->getNameAsString()<<value.str()<<',';
		}
		std::stringstream SSprint;
		if(!SSnotConst.str().empty())
		{
			SSprint<<typeString<<' '<<SSnotConst.str();
			SSprint.seekp(-1,SSprint.cur);
			SSprint<<";\n";
		}
		if(!SSconst.str().empty())
		{
			SSprint<<"const "<<typeString<<' '<<SSconst.str();
			SSprint.seekp(-1,SSprint.cur);
			SSprint<<";\n";
		}
		TheRewriter.RemoveText(SourceRange(declStatement->getBeginLoc(),declStatement->getEndLoc()));
		TheRewriter.InsertText(declStatement->getBeginLoc(),SSprint.str(),true,true);
		//TheRewriter.InsertText(declStatement->getBeginLoc(),SSnotConst.str(),true,true);

	}
*/
		/*	decl=declStatement->getSingleDecl();
	else 	
		decl=declStatement->getDeclGroup().getDeclGroup()[0];

	if(isa<VarDecl>(decl)){
		VarDecl* v= cast<VarDecl>(decl);
		TheRewriter.RemoveText(SourceRange(v->getBeginLoc(),v->getTypeSpecEndLoc()));
		v->setType(v->getType().withConst());
		TheRewriter.InsertText(v->getBeginLoc(),v->getType().getAsString(),false,true);
	}
*/
//	return true;
  //}
  //bool VisitVarDecl(VarDecl *v){
	//std::cout<<v->getNextDeclInContext()<<" "<<v<<"\n";
	//if((v->getNextDeclInContext())!=NULL)
//	{
		//TheRewriter.InsertText(v->getBeginLoc(),"const ",false,true);
		//	v->setType(asString("const"));
//	}
//	return true;
//	}


private:
  Rewriter &TheRewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : Visitor(R){}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    /*for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
	//Constify the program
	VisitorConstify.TraverseDecl(*b);
*/    
for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);

    return true;
  }

private:
  MyASTVisitor Visitor;
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    llvm::errs() << "Usage: rewritersample <filename>\n";
    return 1;
  }

  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  CompilerInstance TheCompInst;
  TheCompInst.createDiagnostics();

  LangOptions &lo = TheCompInst.getLangOpts();
  lo.CPlusPlus = 1;

  // Initialize target info with the default triple for our platform.
  auto TO = std::make_shared<TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI =
      TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
  TheCompInst.setTarget(TI);

  TheCompInst.createFileManager();
  FileManager &FileMgr = TheCompInst.getFileManager();
  TheCompInst.createSourceManager(FileMgr);
  SourceManager &SourceMgr = TheCompInst.getSourceManager();
  TheCompInst.createPreprocessor(TU_Module);
  TheCompInst.createASTContext();

  // A Rewriter helps us manage the code rewriting task.
  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

  // Set the main file handled by the source manager to the input file.
  auto FileInRaw = FileMgr.getFile(argv[1]);
  const FileEntry *FileIn = FileInRaw.get();
  SourceMgr.setMainFileID(
      SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
  TheCompInst.getDiagnosticClient().BeginSourceFile(
      TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());

  // Create an AST consumer instance which is going to get called by
  // ParseAST.
  MyASTConsumer TheConsumer(TheRewriter);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
           TheCompInst.getASTContext());

  // At this point the rewriter's buffer should be full with the rewritten
  // file contents.
  const RewriteBuffer *RewriteBuf =
      TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
  llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());

  return 0;
}
