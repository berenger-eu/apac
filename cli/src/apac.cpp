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

  if (!option) {
    llvm::errs() << llvm::toString(option.takeError()) << "\n";
    return 1;
  }

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

    std::ifstream src(file, std::ios::binary);
    if (!src) {
      llvm::errs() << "Error reading file: " << file << "\n";
      return 1;
    }

    std::ofstream dst(outputFile, std::ios::binary | std::ios::trunc);
    if (!dst) {
      llvm::errs() << "Error creating file: " << outputFile << "\n";
      return 1;
    }

    dst << src.rdbuf();
    src.close();
    dst.close();
    if (!dst) {
      llvm::errs() << "Error writing file: " << outputFile << "\n";
      return 1;
    }
    filesOutput.push_back(outputFile);
  }
  llvm::errs() << "DuplicateFunctionsHandler\n";
  if (DuplicateFunctionsHandler::run(
          option, filesOutput, APACMainFilter.getValue(),
          APACFunctionFilter.getValue(), APACIgnoreFilter.getValue(),
          filesOutput)) {
    return 1;
  }
  llvm::errs() << "ConditionUnstackHandler\n";
  if (ConditionUnstackHandler::run(option, filesOutput,
                                   APACMainFilter.getValue(),
                                   APACFunctionFilter.getValue(),
                                   APACIgnoreFilter.getValue(), filesOutput)) {
    return 1;
  }

  llvm::errs() << "MultipleDeclSplitterHandler\n";
  if (MultipleDeclSplitterHandler::run(
          option, filesOutput, APACMainFilter.getValue(),
          APACFunctionFilter.getValue(), APACIgnoreFilter.getValue(),
          filesOutput)) {
    return 1;
  }
  llvm::errs() << "UnstackHandler\n";
  if (UnstackHandler::run(option, filesOutput, APACMainFilter.getValue(),
                          APACFunctionFilter.getValue(),
                          APACIgnoreFilter.getValue(), filesOutput)) {
    return 1;
  }
  llvm::errs() << "DeclarationSplitterHandler\n";
  if (DeclarationSplitterHandler::run(
          option, filesOutput, APACMainFilter.getValue(),
          APACFunctionFilter.getValue(), APACIgnoreFilter.getValue(),
          filesOutput)) {
    return 1;
  }
  llvm::errs() << "GotoRetHandler\n";
  if (GotoRetHandler::run(option, filesOutput, APACMainFilter.getValue(),
                          APACFunctionFilter.getValue(),
                          APACIgnoreFilter.getValue(), filesOutput)) {
    return 1;
  }
  llvm::errs() << "TaskGraphHandler\n";
  if (TaskGraphHandler::run(option, filesOutput, APACMainFilter.getValue(),
                            APACFunctionFilter.getValue(),
                            APACIgnoreFilter.getValue(), filesOutput)) {
    return 1;
  }

  llvm::errs() << "MainParallelHandler\n";
  if (MainParallelHandler::run(option, filesOutput, APACMainFilter.getValue(),
                               APACFunctionFilter.getValue(),
                               APACIgnoreFilter.getValue(), filesOutput)) {
    return 1;
  }

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