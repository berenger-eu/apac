#include "taskGraph.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
namespace taskGraph {
std::string mainName;
std::vector<std::string> functions, functionsToIgnore;
std::queue<std::string> filesOutputExt;

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R)
      : VisitorTaskGraph(R, orderManager, mainName, functions,
                         functionsToIgnore),
        VisitorDepthAdd(R, mainName, functions, functionsToIgnore),
        TheRewriter(R) {}

  // Parse all the file
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    VisitorDepthAdd.TraverseAST(Ctx);
    SourceManager &sm = TheRewriter.getSourceMgr();
    auto codeBeginLoc = sm.getLocForStartOfFile(sm.getMainFileID());

    auto functions = VisitorDepthAdd.getFunctionsToModify();
    auto returnStmts = VisitorDepthAdd.getReturnStmts();
    VisitorTaskGraph.TraverseAST(Ctx);
    for (auto instr : VisitorTaskGraph.functionsInstructionsVector)
      for (auto instrI : instr)
        instrI.dump();

    llvm::errs() << "Print\n\n";
    VisitorTaskGraph.getAliasTable().dump();
    llvm::errs() << "Print\n\n";

    orderManager.dump();
    auto graphs = generateGraph(VisitorTaskGraph.functionsInstructionsVector,
                                orderManager, VisitorTaskGraph.getAliasTable());

    OutputHandler outputHandler(TheRewriter);
    outputHandler.GenerateDotGraph(graphs, "taskGraphRaw.dot");
    for (auto &graph : graphs) {
      optimizeGraph(graph);
      updateInstructionOrderFromGraph(graph, orderManager);
    }
    outputHandler.GenerateDotGraph(graphs, "taskGraphOpt.dot");
    outputHandler.modifyFile(orderManager);
    modifyCode(TheRewriter, codeBeginLoc, functions, returnStmts);
  }

private:
  StmtOrder orderManager;
  ASTTaskGraphVisitor VisitorTaskGraph;
  ASTDepthAddVisitor VisitorDepthAdd;
  Rewriter &TheRewriter;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();

    if (filesOutputExt.empty())
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    else
      TheRewriter.overwriteChangedFiles();
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};
} // namespace taskGraph
using namespace taskGraph;
bool TaskGraphHandler::run(
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
