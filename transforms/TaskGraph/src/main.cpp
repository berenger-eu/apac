#include "taskGraph.hpp"
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

  return TaskGraphHandler::run(option, files, APACMainFilter.getValue(),
                               APACFunctionFilter.getValue(),
                               APACIgnoreFilter.getValue());
}
