#include "stackheap.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
namespace stackheap {
std::string mainName;
std::vector<std::string> functions;
std::vector<std::string> functionsToIgnore;
std::queue<std::string> filesOutputExt;

struct item_found functionHeap;
struct item_found variableHeap;
// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R)
      : VisitorHeapify(R, functionHeap, variableHeap, mainName, functions,
                       functionsToIgnore),
        TheRewriter(R) {}

  // Override the method that gets called for each parsed top-level
  // declaration.

  // Parse all the AST
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    VisitorHeapify.TraverseAST(Ctx);
    auto &topScopes = VisitorHeapify.getTopScopes();
    auto &scopes = VisitorHeapify.getScopes();
    computeNeededHeap(topScopes);
    std::unordered_map<VarDecl *, std::shared_ptr<item_found>> varToItem;
    for (auto &scope : topScopes)
      computeScopeVariables(scope, 0, varToItem);
    modifyFile(scopes, TheRewriter);
  }

private:
  ASTHeapifyVisitor VisitorHeapify;
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
    if (filesOutputExt.empty())
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    else {
      TheRewriter.overwriteChangedFiles();
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
} // namespace stackheap
using namespace stackheap;

bool HeapifyHandler::run(
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
