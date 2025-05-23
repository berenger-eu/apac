#include "declarationSplitter.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
namespace declarationSplitter {
std::string mainName;
std::vector<std::string> functions;
std::vector<std::string> functionsToIgnore;
std::queue<std::string> filesOutputExt;

std::string stringReferenceHandlerClass() {
  return "#include <functional>\n\
#include <optional>\n\
template <class T>\n\
T& invalid_ref(){\n\
T* ptr = nullptr;\n\
return (*ptr);}\n\n";
}
// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R)
      : VisitorSplitter(R, mainName, functions, functionsToIgnore),
        TheRewriter(R) {}

  // Parse all the file
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    SourceManager &sm = TheRewriter.getSourceMgr();
    TheRewriter.InsertTextAfter(sm.getLocForStartOfFile(sm.getMainFileID()),
                                stringReferenceHandlerClass());
    VisitorSplitter.TraverseAST(Ctx);
  }

private:
  ASTSplitterVisitor VisitorSplitter;
  Rewriter &TheRewriter;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();

    // Now emit the rewritten buffer.
    if (filesOutputExt.empty())
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    else {
      TheRewriter.overwriteChangedFiles();
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
} // namespace declarationSplitter
using namespace declarationSplitter;
bool DeclarationSplitterHandler::run(
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
