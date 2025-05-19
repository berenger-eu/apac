//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "../include/rewritersample.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
std::string mainName;
std::vector<std::string> functions, functionsToIgnore;
std::queue<std::string> filesOutputExt;

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R, SymTab &symTableIn)
      : VisitorInit(R, mainName, functions, functionsToIgnore, symTableIn),
        VisitorConst(R, mainName, functions, functionsToIgnore, symTableIn),
        VisitorPrint(R, mainName, functions, functionsToIgnore, symTableIn) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    // TODO:It would be better to stop Traversing here when doing it in a System
    // Header File

    // First pass, to initialize
    VisitorInit.TraverseAST(Ctx);
    // Constify pass, to calculate dependencies and add const qualifier or not
    VisitorConst.TraverseAST(Ctx);
    // Last pass, to add const where needed in the source file
    VisitorPrint.TraverseAST(Ctx);
  }

private:
  ASTInitVisitor VisitorInit;
  ASTConstifyVisitor VisitorConst;
  ASTPrintVisitor VisitorPrint;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() : SymT(TheRewriter) {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryRefForID(SM.getMainFileID())->getName()
                 << "\n";
    printTextToFiles();
    if (PRINT_TABLE) {
      dumpTableArgs();
    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n\n\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter, SymT);
  }

private:
  void createResultDirectory(std::string folderName) {
    namespace fs = std::filesystem;
    std::error_code err;
    int result = fs::create_directory(folderName, err);
    if (!result && err) {
      llvm::errs() << "Failed to create folder : " << folderName << "\n";
      exit(1);
    } else if (!result) {
      llvm::errs() << "Folder already exists : " << folderName << "\n\n";
    }
  }
  // print modified code to all files (source file and included file(s) )
  void printTextToFiles() {

    // From
    // https://stackoverflow.com/questions/43157172/clang-using-libtooling-rewriter-to-generate-new-file
    std::error_code error_code;
    SourceManager &SM = TheRewriter.getSourceMgr();
    std::string pathToResultsFolder;
    std::string fileName;
    // To print contents of headers files
    for (std::unordered_map<unsigned, FileID>::iterator it =
             SymT.fileID_table.begin();
         it != SymT.fileID_table.end(); ++it) {
      // Parsing the path to the file
      std::string fullPath =
          SM.getFileEntryRefForID(it->second)->getName().str();
      std::stringstream fullPathStream(fullPath);
      if (it == SymT.fileID_table.begin()) {
        std::vector<std::string> separatedString;
        std::string folderName;
        while (getline(fullPathStream, fullPath, '/')) {
          separatedString.push_back(fullPath);
        }
        std::stringstream pathToDir;
        fileName = separatedString.back();
        getline(std::stringstream(fileName), folderName, '.');
        separatedString.pop_back();
        for (std::vector<std::string>::iterator it2 = separatedString.begin();
             it2 != separatedString.end(); it2++) {
          pathToDir << *it2 << "/";
        }
        pathToDir << folderName;
        createResultDirectory(pathToDir.str());
        pathToDir << "/";
        pathToResultsFolder = pathToDir.str();
        llvm::raw_fd_ostream outFile(pathToResultsFolder + fileName, error_code,
                                     llvm::sys::fs::OF_None);
        TheRewriter.getEditBuffer(it->second).write(outFile);
        outFile.close();
      } else {
        while (getline(fullPathStream, fileName, '/'))
          ;
        llvm::raw_fd_ostream outFile(pathToResultsFolder + fileName, error_code,
                                     llvm::sys::fs::OF_None);
        TheRewriter.getEditBuffer(it->second).write(outFile);
        outFile.close();
      }

      llvm::outs() << pathToResultsFolder << " " << fileName;
      llvm::outs() << it->first << ": " << it->second.getHashValue() << "\n";
    }
  }
  std::string getVarName(const_arg &curArg) {
    std::stringstream SSprint;
    if (curArg.declaration) {
      SSprint << curArg.declaration->getNameAsString();
    } else if (curArg.field) {
      SSprint << curArg.field->getNameAsString();
    } else if (curArg.method) {
      SSprint << curArg.method->getNameAsString();
    } else {
      SSprint << "other ";
    }
    return SSprint.str();
  }
  std::string getTypeName(const_arg &curArg) {
    std::stringstream SSprint;
    if (curArg.declaration) {
      SSprint << "Variable : " << curArg.declaration->getType().getAsString();
    } else if (curArg.field) {
      SSprint << "Field : " << curArg.field->getType().getAsString();
    } else if (curArg.method) {
      SSprint << "Method : " << curArg.method->getType().getAsString();
    }
    return SSprint.str();
  }
  void dumpTableArgs() {
    llvm::outs() << "\n\nPrinting table used to store variables\n\n";
    for (std::unordered_map<Decl *, struct const_arg>::iterator it =
             SymT.const_arg_table.begin();
         it != SymT.const_arg_table.end(); ++it) {
      std::stringstream SSprint;
      const_arg &curArg = it->second;
      assert(curArg.declaration || curArg.field || curArg.method);

      SSprint << it->first << " : " << getVarName(curArg);

      if (curArg.is_const) {
        SSprint << " is const,";
      } else {
        SSprint << " is not const,";
      }
      if (curArg.is_ptr_or_ref) {
        SSprint << " is a pointer or a reference,";
      } else {
        SSprint << " is not a pointer or a reference,";
      }

      SSprint << " type is : " << getTypeName(curArg);
      SSprint << "\nDependencies are :\n";
      for (std::vector<const_arg *>::iterator it = curArg.dependencies.begin();
           it != curArg.dependencies.end(); ++it) {
        SSprint << "\t" << getVarName(**it) << "\n";
      }
      SSprint << "\n";
      llvm::outs() << SSprint.str();
    }
    llvm::outs() << "\n\n";
    for (std::unordered_map<Expr *, struct const_arg>::iterator it =
             SymT.const_arg_expr_table.begin();
         it != SymT.const_arg_expr_table.end(); ++it) {
      std::stringstream SSprint;
      const_arg &curArg = it->second;
      // assert(curArg.declaration||curArg.field||curArg.method);

      SSprint << it->first << " : " << getVarName(curArg);

      if (curArg.is_const) {
        SSprint << " is const,";
      } else {
        SSprint << " is not const,";
      }
      if (curArg.is_ptr_or_ref) {
        SSprint << " is a pointer or a reference,";
      } else {
        SSprint << " is not a pointer or a reference,";
      }

      // SSprint<<" type is : "<<getTypeName(curArg);
      SSprint << "\nDependencies are :\n";
      for (std::vector<const_arg *>::iterator it = curArg.dependencies.begin();
           it != curArg.dependencies.end(); ++it) {
        SSprint << "dep:\t" << getVarName(**it) << "\n";
      }
      SSprint << "\n";
      llvm::outs() << SSprint.str();
    }
  }
  Rewriter TheRewriter;
  SymTab SymT;
};

bool ConstifyHandler::run(
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

/*
int main(int argc, char *argv[])
{
        if (argc != 2)
        {
                llvm::errs() << "Usage: rewritersample <filename>\n";
                return 1;
        }

        // CompilerInstance will hold the instance of the Clang compiler for us,
        // managing the various objects needed to run the compiler.
        CompilerInstance TheCompInst;
        TheCompInst.createDiagnostics();
        LangOptions &lo = TheCompInst.getLangOpts();
        lo.CPlusPlus = true;
        lo.CPlusPlus14=true;
        //Header

        HeaderSearchOptions& headSearchOpt=TheCompInst.getHeaderSearchOpts();
        headSearchOpt.AddPath("/usr/include/c++/12",frontend::Angled, false,
false);
        headSearchOpt.AddPath("/usr/include/x86_64-linux-gnu/c++/12",frontend::Angled,
false, false);
        //headSearchOpt.AddPath("/usr/include/c++/12/backward",frontend::Angled,
false, false);
        headSearchOpt.AddPath("/usr/lib/llvm-14/lib/clang/14.0.6/include",frontend::Angled,
false, false);
        //headSearchOpt.AddPath("/usr/local/include",frontend::Angled, false,
false);
        //headSearchOpt.AddPath("/usr/include/x86_64-linux-gnu",frontend::Angled,
false, false); headSearchOpt.AddPath("/usr/include",frontend::Angled, false,
false);





        // Initialize target info with the default triple for our platform.
        auto TO = std::make_shared<TargetOptions>();
        TO->Triple = llvm::sys::getDefaultTargetTriple();
        TargetInfo *TI =
                TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
        TheCompInst.setTarget(TI);

        TheCompInst.createFileManager();
        FileManager &FileMgr = TheCompInst.getFileManager();
        TheCompInst.createSourceManager(FileMgr);
        SourceManager &SourceMgr = TheCompInst.getSourceManager();
        TheCompInst.createPreprocessor(TU_Module);
        TheCompInst.createASTContext();

        // A Rewriter helps us manage the code rewriting task.
        Rewriter TheRewriter;
        TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

        // Set the main file handled by the source manager to the input file.
        auto FileInRaw = FileMgr.getFile(argv[1]);
        const FileEntry *FileIn = FileInRaw.get();
        SourceMgr.setMainFileID(
                SourceMgr.createFileID(FileIn, SourceLocation(),
SrcMgr::C_User)); TheCompInst.getDiagnosticClient().BeginSourceFile(
                TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());
        Preprocessor& prepo=TheCompInst.getPreprocessor();
        // Create an AST consumer instance which is going to get called by
        // ParseAST.
        MyASTConsumer TheConsumer(TheRewriter);

        // Parse the file to AST, registering our consumer as the AST consumer.
        ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
                         TheCompInst.getASTContext());

        // At this point the rewriter's buffer should be full with the rewritten
        // file contents.
        const RewriteBuffer *RewriteBuf =
                TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
        if (RewriteBuf != nullptr)
                llvm::outs() << std::string(RewriteBuf->begin(),
RewriteBuf->end()); else llvm::outs() << "Pas de changements\n"; return 0;
}
*/