#include "gotoRet.hpp"
#include "gotoRetHeader.hpp"
static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");
namespace gotoRet {
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
std::queue<std::string> filesOutputExt;

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
} // namespace gotoRet
using namespace gotoRet;
bool GotoRetHandler::run(
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
