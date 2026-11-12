#include "constifyer.hpp"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::tooling;

namespace {

llvm::cl::OptionCategory ConstifyCategory("Constifyer options");

llvm::cl::opt<std::string>
    APACMainFilter("main", llvm::cl::desc("Main file/function filter"),
                   llvm::cl::init(""), llvm::cl::cat(ConstifyCategory));

llvm::cl::opt<std::string> APACIgnoreFilter("ignore",
                                            llvm::cl::desc("Ignore filter"),
                                            llvm::cl::init(""),
                                            llvm::cl::cat(ConstifyCategory));

llvm::cl::opt<std::string> APACFunctionFilter("functions",
                                              llvm::cl::desc("Function filter"),
                                              llvm::cl::init(""),
                                              llvm::cl::cat(ConstifyCategory));

llvm::cl::opt<bool> VerboseOpt(
    "verbose",
    llvm::cl::desc("Print detailed graph construction and propagation logs"),
    llvm::cl::init(false), llvm::cl::cat(ConstifyCategory));

llvm::cl::opt<bool>
    InPlaceOpt("in-place",
               llvm::cl::desc("Overwrite changed input files instead of "
                              "writing rewritten code to stdout"),
               llvm::cl::init(false), llvm::cl::cat(ConstifyCategory));

/// Simple container for command-line filters.
///
/// Right now this is only passed into the action/consumer.
/// You can later use it inside `shouldTrackDecl`.
struct ConstifyOptions {
  std::string MainFilter;
  std::string FunctionFilter;
  std::string IgnoreFilter;
  bool Verbose = false;
  bool InPlace = false;
  std::vector<std::pair<std::string, std::string>> *Outputs = nullptr;
};

/// ASTConsumer that owns one graph build for one translation unit.
class ConstifyConsumer final : public ASTConsumer {
public:
  ConstifyConsumer(ConstifyOptions Options, std::string File)
      : Options(std::move(Options)), File(std::move(File)) {}

  void HandleTranslationUnit(ASTContext &Ctx) override {
    if (Ctx.getDiagnostics().hasErrorOccurred())
      return;
    constify::Verbose = Options.Verbose;
    constify::InPlace = Options.InPlace;

    constify::ConstGraphBuilder Builder(Ctx);

    Builder.TraverseDecl(Ctx.getTranslationUnitDecl());

    Builder.graph().propagateCannotBeConst();
    Builder.graph().propagateMethodCallConstness();

    if (Options.Verbose)
      Builder.graph().dump(llvm::errs());

    constify::ConstRewritePlanner Planner(Ctx, Builder.graph());
    Planner.compute();
    if (Options.Verbose)
      Planner.dump(llvm::errs());

    constify::ConstRewriteApplier Applier(Ctx, Planner);

    Applier.apply();
    Options.Outputs->emplace_back(File, Applier.sourceText());
  }

private:
  ConstifyOptions Options;
  std::string File;
};

/// FrontendAction created once per translation unit.
class ConstifyFrontendAction final : public ASTFrontendAction {
public:
  explicit ConstifyFrontendAction(ConstifyOptions Options)
      : Options(std::move(Options)) {}

  std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &CI, llvm::StringRef InFile) override {
    if (Options.Verbose) {
      llvm::errs() << "[constify] processing file: " << InFile << "\n";
    }

    return std::make_unique<ConstifyConsumer>(Options, InFile.str());
  }

private:
  ConstifyOptions Options;
};

/// Factory required by ClangTool.
class ConstifyFrontendActionFactory final : public FrontendActionFactory {
public:
  explicit ConstifyFrontendActionFactory(ConstifyOptions Options)
      : Options(std::move(Options)) {}

  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<ConstifyFrontendAction>(Options);
  }

private:
  ConstifyOptions Options;
};

} // namespace

int main(int argc, const char **argv) {
  if (argc < 2) {
    std::cerr
        << "Call with following format:\n"
        << "  ./constifyer <file.cpp> [<file.cpp> ...] -- [compiler flags]\n\n"
        << "Example:\n"
        << "  ./constifyer test.cpp -- -std=c++17\n";
    return 1;
  }

  llvm::Expected<CommonOptionsParser> Option = CommonOptionsParser::create(
      argc, argv, ConstifyCategory, llvm::cl::OneOrMore);

  if (!Option) {
    llvm::errs() << "Could not parse options:\n";
    llvm::errs() << llvm::toString(Option.takeError()) << "\n";
    return 1;
  }

  std::vector<std::string> Files = Option->getSourcePathList();

  for (const auto &File : Files) {
    llvm::errs() << "Input file: " << File << "\n";
  }

  std::vector<std::pair<std::string, std::string>> Outputs;
  ConstifyOptions Options;
  Options.Outputs = &Outputs;
  Options.MainFilter = APACMainFilter.getValue();
  Options.FunctionFilter = APACFunctionFilter.getValue();
  Options.IgnoreFilter = APACIgnoreFilter.getValue();
  Options.Verbose = VerboseOpt.getValue();
  Options.InPlace = InPlaceOpt.getValue();

  if (Options.Verbose) {
    llvm::errs() << "[constify] options:\n";
    llvm::errs() << "  main filter      = \"" << Options.MainFilter << "\"\n";
    llvm::errs() << "  function filter  = \"" << Options.FunctionFilter
                 << "\"\n";
    llvm::errs() << "  ignore filter    = \"" << Options.IgnoreFilter << "\"\n";
    llvm::errs() << "  verbose          = true\n";
  }

  ClangTool Tool(Option->getCompilations(), Files);

  ConstifyFrontendActionFactory Factory(Options);

  if (int Result = Tool.run(&Factory))
    return Result;

  // Check all rewritten translation units in memory before emitting or
  // overwriting any input. An unsupported declarator must never corrupt a
  // file.
  ClangTool Verification(Option->getCompilations(), Files);
  for (const auto &Output : Outputs)
    Verification.mapVirtualFile(Output.first, Output.second);
  if (int Result = Verification.run(
          newFrontendActionFactory<SyntaxOnlyAction>().get())) {
    llvm::errs() << "[constify] rewritten source did not type-check; inputs "
                    "were preserved\n";
    return Result;
  }
  for (const auto &Output : Outputs) {
    if (!Options.InPlace) {
      llvm::outs() << Output.second;
      continue;
    }
    auto Original = llvm::MemoryBuffer::getFile(Output.first);
    if (!Original) {
      llvm::errs() << "[constify] cannot read " << Output.first << ": "
                   << Original.getError().message() << "\n";
      return 1;
    }
    if ((*Original)->getBuffer() == Output.second)
      continue;
    std::error_code Error;
    llvm::raw_fd_ostream Stream(Output.first, Error, llvm::sys::fs::OF_None);
    if (Error) {
      llvm::errs() << "[constify] cannot write " << Output.first << ": "
                   << Error.message() << "\n";
      return 1;
    }
    Stream << Output.second;
    Stream.close();
    if (Stream.has_error()) {
      llvm::errs() << "[constify] failed to write " << Output.first << "\n";
      Stream.clear_error();
      return 1;
    }
  }
  return 0;
}