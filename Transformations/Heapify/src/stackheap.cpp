#include "stackheap.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

std::string mainName;
std::vector<std::string> functions;
std::vector<std::string> functionsToIgnore;

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

bool HeapifyHandler::run(int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "Call with following format : ./stackheap <file.cpp>\n"
              << "\t./stackheap <file.cpp> [-functions -ignore -main -variable "
                 "-variable-function]\n";
    exit(1);
  }

  int argcFiles = (argc - 2 == 2 ? argc - 2 : argc);
  llvm::cl::opt<std::string> APACMainFilter("main");
  llvm::cl::opt<std::string> APACIgnoreFilter("ignore");
  llvm::cl::opt<std::string> APACFunctionFilter("functions");
  llvm::cl::opt<std::string> APACVariableHeapFilter("variable");
  llvm::cl::opt<std::string> APACVariableFunctionHeapFilter("variableFunction");

  llvm::Expected<clang::tooling::CommonOptionsParser> option =
      CommonOptionsParser::create(argcFiles, argv, ToolingSampleCategory,
                                  llvm::cl::OneOrMore);

  auto files = option->getSourcePathList();

  clang::tooling::ClangTool tool(option->getCompilations(), files);
  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  bool transfoSingleVariable =
      (APACVariableHeapFilter.getValue() != "" &&
       APACVariableFunctionHeapFilter.getValue() != "");
  if (transfoSingleVariable) {
    functionHeap.name = APACVariableFunctionHeapFilter.getValue();
    variableHeap.name = APACVariableFunctionHeapFilter.getValue();
    functionHeap.found = false;
    variableHeap.found = false;
  } else {
    callParse(APACMainFilter.getValue(), APACFunctionFilter.getValue(),
              APACIgnoreFilter.getValue(), mainName, functionsToIgnore,
              functions);
  }
  int retVal = tool.run(newFrontendActionFactory<MyFrontendAction>().get());
  // Only when looking for a specific function and variable
  if (transfoSingleVariable) {
    std::stringstream SSprint;
    if (!functionHeap.found) {
      SSprint << "\nFunction not found\n ";
    } else if (!variableHeap.found) {
      SSprint << "\nVariable not found\n";
    }
    if (!SSprint.str().empty()) {
      std::cerr << SSprint.str();
      exit(1);
    }
  }
  return retVal;
}

int main(int argc, const char **argv) {
  return HeapifyHandler::run(argc, argv);
}
