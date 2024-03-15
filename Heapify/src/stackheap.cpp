#include "stackheap.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;


// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer
{
public:
	MyASTConsumer(Rewriter &R) : VisitorHeapify(R) {}

	// Override the method that gets called for each parsed top-level
	// declaration.
	virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
    {   
      Decl* dec=*b;
      if(foundCorrectFunction(*dec))
          VisitorHeapify.TraverseDecl(dec);
        // Traverse the declaration using our AST visitor.
            
    }
    return true;
  }

private:
	ASTHeapifyVisitor VisitorHeapify;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

    // Now emit the rewritten buffer.
    TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  if(argc!=4&&argc!=2){
    std::cerr<<"Call with following format : ./stackheap <file.cpp>\n"
    <<"\t./stackheap <file.cpp> <function> <variable>\n";
    exit(1);
  }
  
  int argcFiles=(argc-2==2?argc-2:argc);
  llvm::Expected<clang::tooling::CommonOptionsParser> option = CommonOptionsParser::create
  (argcFiles, argv, ToolingSampleCategory, llvm::cl::OneOrMore);

auto files = option->getSourcePathList();

clang::tooling::ClangTool tool(option->getCompilations(), files);
  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
    if(argc==4)
    {
      std::string s1(argv[argc-2]);
      std::string s2(argv[argc-1]);
      functionHeap.name=s1;
      variableHeap.name=s2;
      functionHeap.found=false;
      variableHeap.found=false;
    }
    int retVal=tool.run(newFrontendActionFactory<MyFrontendAction>().get());
    //Only when looking for a specific function and variable
    if(argc==4)
    {
      std::stringstream SSprint;
      if(!functionHeap.found)
        SSprint<<"\nFunction not found\n ";
      else if(!variableHeap.found)
        SSprint<<"\nVariable not found\n";
      if(!SSprint.str().empty())
      {
        std::cerr<<SSprint.str();
        exit(1);
      }
    }  
    return retVal;
}
