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


static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;


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
		//TODO:It would be better to stop Traversing here when doing it in a System Header File
		
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

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";
	printTextToFiles(); 

  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
	void createResultDirectory(std::string folderName)
	{
		std::string command="mkdir "+folderName;
		int result = system(command.c_str());
		if (!result) 
		{
			llvm::errs()<<"Failed to create folder : "<<folderName<<"\n";
			exit(1);      
		}	
	}
	//print modified code to all files (source file and included file(s) )
	void printTextToFiles()
	{

	//From https://stackoverflow.com/questions/43157172/clang-using-libtooling-rewriter-to-generate-new-file
	std::error_code error_code;
	SourceManager& SM=TheRewriter.getSourceMgr();
	std::string pathToResultsFolder;
	std::string fileName;
	//To print contents of headers files
	for (std::unordered_map<unsigned, FileID>::iterator it = fileID_table.begin(); it != fileID_table.end(); ++it) {
        //Parsing the path to the file
		std::string fullPath=SM.getFileEntryForID(it->second)->getName().str();
		std::stringstream fullPathStream(fullPath);
		if(it==fileID_table.begin())
		{
			std::vector<std::string> separatedString;
			std::string folderName;
			while (getline(fullPathStream, fullPath, '/')) 
				separatedString.push_back(fullPath);
			std::stringstream pathToDir;
			fileName=separatedString.back();
			getline(std::stringstream(fileName),folderName,'.');
			separatedString.pop_back();
			for(std::vector<std::string>::iterator it2=separatedString.begin();it2!=separatedString.end();it2++)
				pathToDir<<*it2<<"/";
			pathToDir<<folderName;
			createResultDirectory(pathToDir.str());
			pathToDir<<"/";
			pathToResultsFolder=pathToDir.str();
			llvm::raw_fd_ostream outFile(pathToResultsFolder+fileName, error_code, llvm::sys::fs::OF_None);
			TheRewriter.getEditBuffer(it->second).write(outFile);
			outFile.close();
		}
		else {
			while (getline(fullPathStream, fileName, '/')); 
			llvm::raw_fd_ostream outFile(pathToResultsFolder+fileName, error_code, llvm::sys::fs::OF_None);
			TheRewriter.getEditBuffer(it->second).write(outFile);
			outFile.close();
		}
    	
		llvm::outs()<<pathToResultsFolder<<" "<<fileName;
		llvm::outs()<< it->first << ": " << it->second.getHashValue() << "\n";
    }
	llvm::outs()<<fileID_table.size();
	}
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  
  llvm::Expected<clang::tooling::CommonOptionsParser> option = CommonOptionsParser::create
  (argc, argv, ToolingSampleCategory, llvm::cl::OneOrMore);

auto files = option->getSourcePathList();

clang::tooling::ClangTool tool(option->getCompilations(), files);
  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  return tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}




/*
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
	
	HeaderSearchOptions& headSearchOpt=TheCompInst.getHeaderSearchOpts();
	headSearchOpt.AddPath("/usr/include/c++/12",frontend::Angled, false, false);
	headSearchOpt.AddPath("/usr/include/x86_64-linux-gnu/c++/12",frontend::Angled, false, false);
	//headSearchOpt.AddPath("/usr/include/c++/12/backward",frontend::Angled, false, false);
	headSearchOpt.AddPath("/usr/lib/llvm-14/lib/clang/14.0.6/include",frontend::Angled, false, false);
	//headSearchOpt.AddPath("/usr/local/include",frontend::Angled, false, false);
	//headSearchOpt.AddPath("/usr/include/x86_64-linux-gnu",frontend::Angled, false, false);
	headSearchOpt.AddPath("/usr/include",frontend::Angled, false, false);
	
 
 
	

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
*/