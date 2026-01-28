#include "apac.hpp"
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
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
  /*
  callParse(APACMainFilter.getValue(), APACFunctionFilter.getValue(),
            APACIgnoreFilter.getValue(), mainName, functionsToIgnore,
            functions);
  */
  std::vector<std::string> filesOutput;
  for (auto file : files) {
    std::string outputFile = file;
    size_t lastSlash = outputFile.find_last_of('/');
    if (lastSlash != std::string::npos) {
      outputFile.insert(lastSlash + 1, "APAC");
    } else {
      outputFile = "APAC" + outputFile;
    }

    // Empty the output file beforehand
    std::ofstream dst(outputFile, std::ios::binary | std::ios::trunc);
    if (!dst) {
      llvm::errs() << "Error creating file: " << outputFile << "\n";
      continue;
    }

    std::ifstream src(file, std::ios::binary);
    if (!src) {
      llvm::errs() << "Error reading file: " << file << "\n";
      continue;
    }

    dst << src.rdbuf();
    src.close();
    dst.close();
    filesOutput.push_back(outputFile);
  }
  llvm::errs() << "DuplicateFunctionsHandler\n";
  DuplicateFunctionsHandler::run(option, filesOutput, APACMainFilter.getValue(),
                                 APACFunctionFilter.getValue(),
                                 APACIgnoreFilter.getValue(), filesOutput);
  llvm::errs() << "ConditionUnstackHandler\n";
  ConditionUnstackHandler::run(option, filesOutput, APACMainFilter.getValue(),
                               APACFunctionFilter.getValue(),
                               APACIgnoreFilter.getValue(), filesOutput);

  llvm::errs() << "MultipleDeclSplitterHandler\n";
  MultipleDeclSplitterHandler::run(
      option, filesOutput, APACMainFilter.getValue(),
      APACFunctionFilter.getValue(), APACIgnoreFilter.getValue(), filesOutput);
  llvm::errs() << "UnstackHandler\n";
  UnstackHandler::run(option, files, APACMainFilter.getValue(),
                      APACFunctionFilter.getValue(),
                      APACIgnoreFilter.getValue(), filesOutput);
  llvm::errs() << "DeclarationSplitterHandler\n";
  DeclarationSplitterHandler::run(
      option, filesOutput, APACMainFilter.getValue(),
      APACFunctionFilter.getValue(), APACIgnoreFilter.getValue(), filesOutput);
  llvm::errs() << "GotoRetHandler\n";
  GotoRetHandler::run(option, filesOutput, APACMainFilter.getValue(),
                      APACFunctionFilter.getValue(),
                      APACIgnoreFilter.getValue(), filesOutput);
  llvm::errs() << "TaskGraphHandler\n";
  TaskGraphHandler::run(option, filesOutput, APACMainFilter.getValue(),
                        APACFunctionFilter.getValue(),
                        APACIgnoreFilter.getValue(), filesOutput);

  llvm::errs() << "MainParallelHandler\n";
  MainParallelHandler::run(option, filesOutput, APACMainFilter.getValue(),
                           APACFunctionFilter.getValue(),
                           APACIgnoreFilter.getValue(), filesOutput);

  /*
  StackHeapHandler::run(option, files, APACMainFilter.getValue(),
  APACFunctionFilter.getValue(),
  APACIgnoreFilter.getValue());
  */

  /*

#run_const $curUsedFile
#run_transformation $unstackTransfo $unstackName $curUsedFile $curUsedFile
run_transformation $declSplitTransfo $declSplitName $curUsedFile
run_transformation $gotoTransfo $gotoName $curUsedFile
#run_transformation $stackHeapTransfo $stackHeapName $curUsedFile
run_transformation $taskGraphTransfo $taskGraphName
$curUsedFile run_transformation $mainParallel
$mainParallelName $curUsedFile
*/
  return 0;
}