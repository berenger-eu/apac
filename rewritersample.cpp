//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "rewritersample.h"
using namespace clang;
std::unordered_map<std::string,struct const_arg> const_arg_table;



class MyFirstASTPassVisitor : public RecursiveASTVisitor<MyFirstASTPassVisitor> {
		public:
				MyFirstASTPassVisitor (Rewriter &R) : TheRewriter(R) {}
		bool VisitStmt(Stmt *s){
				return true;
		}
/*		bool VisitFunctionDecl(FunctionDecl *f) {
		// Only function definitions (with bodies), not declarations.
				for(FunctionDecl::param_iterator b=f->param_begin(),e=f->param_end();b!=e;++b)
				{
						ParmVarDecl* pv=*b;
						pv->setType(pv->getType().withConst());
				}
				return true;
		}
*/
		bool VisitVarDecl(VarDecl* v) {
			
			Expr* expr=v->getInit();
			DeclRefExpr* dr=NULL;
			if (expr!=NULL&&isa<DeclRefExpr>(expr->IgnoreCasts()))
			dr=cast<DeclRefExpr>(expr->IgnoreCasts());
				const_arg& curDeclArg=const_arg_table[v->getQualifiedNameAsString()];
				curDeclArg.is_const=true;
				curDeclArg.declaration=v;
				QualType qt=v->getType();
				const Type* intype=qt.getTypePtrOrNull();
				if(intype!=NULL)
				{
						
					if(intype->isPointerType())
					{
						curDeclArg.is_ptr_or_ref=true;

					}
					else if(intype->isReferenceType())
					{
						
						curDeclArg.is_ptr_or_ref=true;
						if(dr!=NULL)
						curDeclArg.dependencies=&(const_arg_table[
								cast<NamedDecl>(dr->getDecl())->getQualifiedNameAsString()]);
						//std::stringstream SSprint;
						//SSprint<<"\n"<<temp->getQualifiedNameAsString()<<"\n";
						//TheRewriter.InsertText(v->getEndLoc(),SSprint.str(),true,true);
						//curDeclArg.dependencies=&const_arg_table[dependence];
						//llvm::outs() << temp->getQualifiedNameAsString()<<"\n";
					}
				}
				return true;
		}
		bool VisitBinaryOperator(BinaryOperator* bop)
		{
			if(bop->isAssignmentOp())
			{
				DeclRefExpr* leftSide;//,rightSide;
				leftSide=cast<DeclRefExpr>(bop->getLHS());
				//rightSide=cast<DeclRefExpr>(bop->getRHS());
				std::string qualifiedLeftName=leftSide->getDecl()->getQualifiedNameAsString();
				const_arg* curArg=&(const_arg_table[qualifiedLeftName]);
				curArg->is_const=false;
				QualType t=curArg->declaration->getType();
				t.removeLocalConst();
				curArg->declaration->setType(t);
				int i=0;
				while(curArg->dependencies!=NULL&&i<100)

				{
					i++;
					curArg=curArg->dependencies;
					curArg->is_const=false;
					QualType t=curArg->declaration->getType();
					t.removeLocalConst();
					curArg->declaration->setType(t);

				}
			}
			return true;
		}
		bool VisitDeclRefExpr(DeclRefExpr* de)
		{
			VarDecl* v=dyn_cast<VarDecl>(de->getDecl());
			if(v)
			{
			}
			return true;
		}
		private:
  Rewriter &TheRewriter;				
};

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

  bool VisitStmt(Stmt *s) {
    // Only care about If statements.
    if (isa<IfStmt>(s)) {
      IfStmt *IfStatement = cast<IfStmt>(s);
      Stmt *Then = IfStatement->getThen();

      TheRewriter.InsertText(Then->getBeginLoc(), "// the 'if' part\n", true,
                             true);

      Stmt *Else = IfStatement->getElse();
      if (Else)
        TheRewriter.InsertText(Else->getBeginLoc(), "// the 'else' part\n",
                               true, true);
    }
    return true;
  }




  bool VisitFunctionDecl(FunctionDecl *f) {
    // Only function definitions (with bodies), not declarations.
    for(FunctionDecl::param_iterator b=f->param_begin(),e=f->param_end();b!=e;++b)
    {
	    ParmVarDecl* pv=*b;

	    QualType qt=pv->getType();
	    const Type* intype=qt.getTypePtrOrNull();
	    
	    if(intype!=NULL)

	    {
		    if(intype->isBuiltinType())
		    {
  			TheRewriter.ReplaceText(SourceRange(pv->getTypeSpecStartLoc(),
			pv->getTypeSpecEndLoc()),pv->getType().getAsString());	
		    }

		    else if(intype->isPointerType())
		    {
			QualType inpv=intype->getPointeeType();
			inpv.addConst();		
		    }
		    else if(intype->isReferenceType())
		    {
		//	QualType qtNew(QualType(qt.getTypePtr(),qt.getNonReferenceType().getLocalFastQualifiers()),qt.getLocalFastQualifiers());	
/*			QualType qtNew=(qt.getNonReferenceType().getTypePtr(),qt.getNonReferenceType());
			    llvm::outs()<<"Nouveau QualType\n";
			const ReferenceType* tp=cast<ReferenceType>(qtNew.getTypePtr());
			    ReferenceType rtf=(tp->getTypeClass(),qt.getNonReferenceType().withConst(),qt.getNonReferenceType().getCanonicalType().withConst(),*tp);
			    qtNew.dump();
			    llvm::outs()<<"QualType Bis\n";
			llvm::outs()<<"\nAncien QualType\n";
			
			qt.dump();
			llvm::outs()<<"\n";
			    qt.getTypePtr()->dump();
			llvm::outs()<<"\n";
			//cast<QualType>(qtNew.getTypePtr())->addConst();
		*/
//			    llvm::outs()<<std::to_string(&qt)<<" \n"<<std::to_string(qt.getTypePtr())<<"\n";
			//	llvm::outs()<<"Au dessous\n"<<qtNew.getNonReferenceType().isConstQualified()<<"\nAu dessus\n";
		    QualType qtIn=qt.getNonReferenceType();
		    qtIn.addConst();
		    ASTContext& acons=pv->getASTContext();
		    pv->setType(acons.getLValueReferenceType(qtIn));
		    }
	    }	    
	    //TheRewriter.RemoveText(SourceRange(pv->getTypeSpecStartLoc(),pv->getTypeSpecEndLoc()));
    
    }
    return true;
  }
  bool VisitVarDecl(VarDecl* v)
  {
		/*const_arg tempArg=const_arg_table[v->getQualifiedNameAsString()];
		if(tempArg.is_const)
		{	
			Type* tpv;
			if((v->getType().getTypePtrOrNull())!=NULL)
			{
				TheRewriter.InsertText(tpv
			}
				std::stringstream SSprint;
			SSprint<<"const "<<v->getType().getAsString();
		*/	//TheRewriter.ReplaceText(SourceRange(v->getTypeSpecStartLoc(),v->getTypeSpecEndLoc()),SSprint.str());	
		
			TheRewriter.ReplaceText(SourceRange(v->getTypeSpecStartLoc(),v->getTypeSpecEndLoc()),v->getType().getAsString());	

		  return true;
  }
private:
  Rewriter &TheRewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : Visitor(R),VisitorFirstPass(R)  {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    
    for (DeclGroupRef::iterator b=DR.begin(),e=DR.end();b!=e;++b)
		VisitorFirstPass.TraverseDecl(*b);
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) 
	    // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
    return true;
  }

private:
  MyASTVisitor Visitor;
  MyFirstASTPassVisitor VisitorFirstPass;
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
  if(RewriteBuf!=NULL)llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());
  else llvm::outs() <<"Pas de changements\n";
  return 0;
}
