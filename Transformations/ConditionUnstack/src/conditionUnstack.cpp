#include "conditionUnstack.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
std::string mainName;
std::vector<std::string> functions, functionsToIgnore;
// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R)
      : VisitorConditionUnstack(R, mainName, functions, functionsToIgnore),
        TheRewriter(R) {}

  // Parse all the AST
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    VisitorConditionUnstack.TraverseAST(Ctx);
  }

private:
  ASTConditionUnstackVisitor VisitorConditionUnstack;
  Rewriter &TheRewriter;
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

bool ConditionUnstackHandler::run(int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "Call with following format : ./conditionUnstack <file.cpp> "
                 "[<file.cpp> ...]\n";
    exit(1);
  }
  llvm::cl::opt<std::string> APACMainFilter("main");
  llvm::cl::opt<std::string> APACIgnoreFilter("ignore");
  llvm::cl::opt<std::string> APACFunctionFilter("functions");
  llvm::Expected<clang::tooling::CommonOptionsParser> option =
      CommonOptionsParser::create(argc, argv, ToolingSampleCategory,
                                  llvm::cl::OneOrMore);

  auto files = option->getSourcePathList();

  clang::tooling::ClangTool tool(option->getCompilations(), files);
  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  callParse(APACMainFilter.getValue(), APACFunctionFilter.getValue(),
            APACIgnoreFilter.getValue(), mainName, functionsToIgnore,
            functions);
  return tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}

int main(int argc, const char **argv) {
  return ConditionUnstackHandler::run(argc, argv);
}
