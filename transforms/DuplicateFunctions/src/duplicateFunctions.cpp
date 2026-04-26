#include "duplicateFunctions.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
namespace duplicateFunctions {
std::string mainName;
std::vector<std::string> functions, functionsToIgnore;
std::queue<std::string> filesOutputExt;

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R)
      : VisitorDuplicateFunc(R, mainName, functions, functionsToIgnore),
        VisitorChangeName(R, mainName, functions, functionsToIgnore) {}
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    VisitorDuplicateFunc.TraverseAST(Ctx);
    VisitorDuplicateFunc.addDuplicateFunctions();
    VisitorChangeName.TraverseAST(Ctx);
  }

private:
  ASTDuplicateVisitor VisitorDuplicateFunc;
  ASTChangeNameVisitor VisitorChangeName;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();

    // Now emit the rewritten buffer.*
    if (filesOutputExt.empty())
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    else {
      // std::error_code error_code;
      //  llvm::raw_fd_ostream outFile(filesOutputExt.front(), error_code,
      //                               llvm::sys::fs::OF_None);
      (void)TheRewriter.overwriteChangedFiles();
      // TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);
      // outFile.close();
      // filesOutputExt.pop();
    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};
} // namespace duplicateFunctions
using namespace duplicateFunctions;
bool DuplicateFunctionsHandler::run(
    llvm::Expected<clang::tooling::CommonOptionsParser> &options,
    std::vector<std::string> &filesInput, const std::string &mainFilterValue,
    const std::string &functionsFilterValue,
    const std::string &functionsIgnoreValue,
    const std::vector<std::string> &filesOutput) {
  callParse(mainFilterValue, functionsFilterValue, functionsIgnoreValue,
            mainName, functionsToIgnore, functions);
  for (auto file : filesOutput) {
    filesOutputExt.push(file);
  }
  clang::tooling::ClangTool tool(options->getCompilations(), filesInput);
  return tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
