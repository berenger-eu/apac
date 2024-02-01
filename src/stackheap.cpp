#include "../include/stackheap.hpp"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;


bool isArrayVariable(VarDecl* v)
{
  return ( v->getType().getTypePtrOrNull()!=NULL && v->getType().getTypePtrOrNull()->isArrayType() );
}

//returns the string containing the Init part of a variable (Variable is supposed to be init here)
std::string createInitString(VarDecl* v)
{
  assert(v->getInit()!=NULL);
  PrintingPolicy print_policy(v->getASTContext().getLangOpts());
  std::string initString;
  llvm::raw_string_ostream stringStreamInit(initString);
  v->getInit()->printPretty(stringStreamInit,NULL,print_policy);
  return initString;
}
//Builds the string to delete a variable
std::string createDeleteString()
{
  std::stringstream SSprint;
  SSprint<<"delete";
  if(variableHeap.array)
    SSprint<<"[] "<<variableHeap.name<<";\n";
  else
    SSprint<<" apacMemeBloc;\n";
  return SSprint.str();
}
//Build the string to create the new variable 
std::string createCreationString(VarDecl* v)
{
  std::stringstream SSprint;
  std::string vType=v->getType().getAsString();
  if(isArrayVariable(v))
  {
    //type* varname = new type[N]
    SSprint<<v->getType().getTypePtrOrNull()->getAsArrayTypeUnsafe()->getElementType().getAsString()<<"* "
    <<v->getNameAsString()<<" = new "<<vType;
    if(v->getInit()!=NULL)
      SSprint<<createInitString(v);
  }
  else
  {
    SSprint<<vType<<"*apacMemeBloc = new "
    <<vType;
    if(v->getInit()!=NULL)
    {
      SSprint<<'('<<createInitString(v)<<')';
    }
    else
      SSprint<<"()";
    SSprint<<";\n"<<vType<<"& "<<v->getNameAsString()<<"= *(apacMemeBloc)\n";
    
  }
  return SSprint.str();
}

bool ASTHeapifyVisitor::VisitStmt(Stmt *s)
{
    return true;
}
//To see if the function was found (mostly for debugging)
bool ASTHeapifyVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
    if(fDecl->getNameAsString().compare(functionHeap.name)==0)
    {
        functionHeap.found=true;
    }
    return true;
}
//Visits each scope
bool ASTHeapifyVisitor::VisitCompoundStmt(CompoundStmt* coSt)
{
  bool deleteEnd=false; //If we have to delete at the end of the scope (so if there is no return)
  bool curState=variableHeap.found; //If variable is found, we're in a subLoop, so no delete (unless there is a return) 
  for (CompoundStmt::body_iterator b = coSt->body_begin(), e = coSt->body_end(); b != e; ++b)
  {
    Stmt* st=*b;
    
    if(isa<ReturnStmt>(st))
    {
      ReturnStmt* retStmt=cast<ReturnStmt>(st);
      subVisitReturnStmt(retStmt);
      deleteEnd=false;  //No need to delete at the end of the scope if there is a return (and so a delete)
    }
    else if (isa<DeclStmt>(st))
    {
      DeclStmt* decStmt=cast<DeclStmt>(st);
      Decl* dec;
      if(decStmt->isSingleDecl())
        dec=decStmt->getSingleDecl();
      if(dec!=NULL && isa<VarDecl>(dec))
        subVisitVarDecl(cast<VarDecl>(dec));
      //True when variable was found in the current scope, so we have to add a delete at the end of it
      if(curState!=variableHeap.found)
      {
        curState=!curState;
        deleteEnd=true;
      }
    }
  }
  if(deleteEnd)
      TheRewriter.InsertTextAfter(coSt->getEndLoc(),createDeleteString()); 
  return true;
}
bool ASTHeapifyVisitor::subVisitVarDecl(VarDecl* v)
{     
  //True when VarDecl corresponds to the searched variable
  if(v->getNameAsString().compare(variableHeap.name)==0)
  {
    variableHeap.found=true;
    variableHeap.array=isArrayVariable(v);
    TheRewriter.ReplaceText(SourceRange(v->getBeginLoc(),v->getEndLoc()),createCreationString(v));
  }
  return true;
}
bool ASTHeapifyVisitor::subVisitReturnStmt(ReturnStmt* retStmt)
{
    //We have to delete the variable when it has been found (and thus created)
    if(variableHeap.found)
    {
        TheRewriter.InsertText(retStmt->getBeginLoc(),createDeleteString());
    }
    return true;
}

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.

class MyASTConsumer : public ASTConsumer
{
public:
	MyASTConsumer(Rewriter &R) : VisitorHeapify(R) {}

	// Override the method that gets called for each parsed top-level
	// declaration.
	virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
    {   
        Decl* dec=*b;
        if(isa<FunctionDecl>(dec))
        {
            FunctionDecl* fDecl = cast<FunctionDecl>(dec);
            if(fDecl->getNameAsString().compare(functionHeap.name)==0)
                VisitorHeapify.TraverseDecl(dec);
        }
        // Traverse the declaration using our AST visitor.
            
    }
    return true;
  }

private:
	ASTHeapifyVisitor VisitorHeapify;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

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

int main(int argc, const char **argv) {
  if(argc!=4){
    std::cerr<<"Nombre d'arguments incorrect\n";
    exit(1);
  }
  
  int argcFiles=argc-2;
  llvm::Expected<clang::tooling::CommonOptionsParser> option = CommonOptionsParser::create
  (argcFiles, argv, ToolingSampleCategory, llvm::cl::OneOrMore);

auto files = option->getSourcePathList();

clang::tooling::ClangTool tool(option->getCompilations(), files);
  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
    std::string s1(argv[argc-2]);
    std::string s2(argv[argc-1]);
    functionHeap.name=s1;
    variableHeap.name=s2;
    functionHeap.found=false;
    variableHeap.found=false;
   int retVal=tool.run(newFrontendActionFactory<MyFrontendAction>().get());
   std::stringstream SSprint;
    if(!functionHeap.found)
        SSprint<<"\nFunction not found\n ";
    else if(!variableHeap.found)
        SSprint<<"\nVariable not found\n";
    if(!SSprint.str().empty())
    {
      std::cerr<<SSprint.str();
      exit(1);
    }
    return retVal;
}
