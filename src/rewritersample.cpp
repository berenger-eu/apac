//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "../include/rewritersample.hpp"

using namespace clang;

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer
{
public:
	MyASTConsumer(Rewriter &R) : VisitorInit(R),VisitorConst(R) ,VisitorPrint(R) {}

	// Override the method that gets called for each parsed top-level
	// declaration.
	virtual void HandleTranslationUnit(ASTContext &Ctx)
	{
		// First pass, to initialize
			VisitorInit.TraverseAST(Ctx);
		// Constify pass, to calculate dependencies and add const qualifier or not
			VisitorConst.TraverseAST(Ctx);			
		// Last pass, to add const where needed in the source file
			VisitorPrint.TraverseAST(Ctx);
	}

private:
	ASTInitVisitor VisitorInit;
	ASTConstifyVisitor VisitorConst;
	ASTPrintVisitor VisitorPrint;
};

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		llvm::errs() << "Usage: rewritersample <filename>\n";
		return 1;
	}

	// CompilerInstance will hold the instance of the Clang compiler for us,
	// managing the various objects needed to run the compiler.
	CompilerInstance TheCompInst;
	TheCompInst.createDiagnostics();
	LangOptions &lo = TheCompInst.getLangOpts();
	lo.CPlusPlus = true;
	lo.CPlusPlus14=true;
	//Header
	/*
	HeaderSearchOptions& headSearchOpt=TheCompInst.getHeaderSearchOpts();
	headSearchOpt.AddPath("/usr/include/c++/12",frontend::Angled, false, false);
	headSearchOpt.AddPath("/usr/include/x86_64-linux-gnu/c++/12",frontend::Angled, false, false);
	//headSearchOpt.AddPath("/usr/include/c++/12/backward",frontend::Angled, false, false);
	headSearchOpt.AddPath("/usr/lib/llvm-14/lib/clang/14.0.6/include",frontend::Angled, false, false);
	//headSearchOpt.AddPath("/usr/local/include",frontend::Angled, false, false);
	//headSearchOpt.AddPath("/usr/include/x86_64-linux-gnu",frontend::Angled, false, false);
	headSearchOpt.AddPath("/usr/include",frontend::Angled, false, false);
	*/
 
 
	

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
	Preprocessor& prepo=TheCompInst.getPreprocessor();
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
	if (RewriteBuf != NULL)
		llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());
	else
		llvm::outs() << "Pas de changements\n";
	return 0;
}
