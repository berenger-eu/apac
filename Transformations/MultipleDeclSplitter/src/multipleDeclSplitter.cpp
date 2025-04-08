#include "multipleDeclSplitter.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

std::string mainName;
std::vector<std::string> functions, functionsToIgnore;
std::queue<std::string> filesOutputExt;
// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R)
      : VisitorSplitter(R, mainName, functions, functionsToIgnore) {}

  // Parse all the file
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    VisitorSplitter.TraverseAST(Ctx);
  }

private:
  ASTMultipleDeclSplitter VisitorSplitter;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryRefForID(SM.getMainFileID())->getName()
                 << "\n";

    // Now emit the rewritten buffer.

    if (filesOutputExt.empty())
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    else {
      std::error_code error_code;
      llvm::raw_fd_ostream outFile(filesOutputExt.front(), error_code,
                                   llvm::sys::fs::OF_None);
      filesOutputExt.pop();
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);
    }
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

bool MultipleDeclSplitterHandler::run(
    llvm::Expected<clang::tooling::CommonOptionsParser> &options,
    std::vector<std::string> &filesInput,
    const std::vector<std::string> &filesOutput) {
  for (auto file : filesOutput) {
    filesOutputExt.push(file);
  }
  clang::tooling::ClangTool tool(options->getCompilations(), filesInput);
  return tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}

int main(int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "Call with following format : ./prog <file.cpp> "
                 "[<file.cpp> ...]\n";
    exit(1);
  }
  llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");
  // The following lines are used to define the options that can be passed
  // Template type for the type of the argument
  // The init string is the name of the option
  llvm::cl::opt<std::string> APACMainFilter("main");
  llvm::cl::opt<std::string> APACIgnoreFilter("ignore");
  llvm::cl::opt<std::string> APACFunctionFilter("functions");

  llvm::Expected<clang::tooling::CommonOptionsParser> option =
      CommonOptionsParser::create(argc, argv, ToolingSampleCategory,
                                  llvm::cl::OneOrMore);

  auto files = option->getSourcePathList();
  for (auto file : files) {
    llvm::errs() << file << "\n";
  }
  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  callParse(APACMainFilter.getValue(), APACFunctionFilter.getValue(),
            APACIgnoreFilter.getValue(), mainName, functionsToIgnore,
            functions);
  return MultipleDeclSplitterHandler::run(option, files);
}
