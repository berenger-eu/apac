#include "taskGraph.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

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
      : VisitorTaskGraph(R, orderManager), VisitorDepthAdd(R), TheRewriter(R) {}

  // Parse all the file
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    VisitorDepthAdd.TraverseAST(Ctx);
    SourceManager &sm = TheRewriter.getSourceMgr();
    auto codeBeginLoc = sm.getLocForStartOfFile(sm.getMainFileID());

    auto functions = VisitorDepthAdd.getFunctionsToModify();
    modifyCode(TheRewriter, codeBeginLoc, functions);
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
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryRefForID(SM.getMainFileID())->getName()
                 << "\n";
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
  if (argc < 2) {
    std::cerr << "Call with following format : ./taskGraph <file.cpp> "
                 "[<file.cpp> ...]\n";
    exit(1);
  }
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
  return tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
