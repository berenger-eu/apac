#include "gotoRet.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

void printTextToFiles(Rewriter &TheRewriter, const FileID &file) {

  // From
  // https://stackoverflow.com/questions/43157172/clang-using-libtooling-rewriter-to-generate-new-file
  std::error_code error_code;
  SourceManager &SM = TheRewriter.getSourceMgr();
  std::string fileName = "_apac_header.hpp";
  // To print contents of headers files

  // Parsing the path to the file
  std::string fullPath = SM.getFileEntryRefForID(file)->getName().str();
  std::stringstream fullPathStream(fullPath);
  std::vector<std::string> separatedString;
  while (getline(fullPathStream, fullPath, '/')) {
    separatedString.push_back(fullPath);
  }
  std::stringstream pathToDir;
  separatedString.pop_back();
  for (std::vector<std::string>::iterator it2 = separatedString.begin();
       it2 != separatedString.end(); it2++) {
    pathToDir << *it2 << "/";
  }
  pathToDir << fileName;
  llvm::raw_fd_ostream outFile(pathToDir.str(), error_code,
                               llvm::sys::fs::OF_None);
  outFile << gotoHeader;
  outFile.close();
}
std::string mainName;
std::vector<std::string> functions, functionsToIgnore;
// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R)
      : VisitorGoto(R, mainName, functions, functionsToIgnore), TheRewriter(R) {
  }

  // Parse all the AST
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    SourceManager &SM = TheRewriter.getSourceMgr();
    TheRewriter.InsertTextAfter(SM.getLocForStartOfFile(SM.getMainFileID()),
                                "#include \"_apac_header.hpp\"\n");
    printTextToFiles(TheRewriter, SM.getMainFileID());
    VisitorGoto.TraverseAST(Ctx);
  }

private:
  ASTGotoVisitor VisitorGoto;
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

bool GotoRetHandler::run(int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "Call with following format : ./gotoRet <file.cpp> "
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
  return GotoRetHandler::run(argc, argv);
}
