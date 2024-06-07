#include "ASTHeapifyVisitor.hpp"
using namespace clang;

bool ASTHeapifyVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
//In case we were trying to look for a function defined in the header
  if(isInHeaders(TheRewriter.getSourceMgr(),fDecl->getEndLoc()))
      return true;
  functionHeap.found=true;
  subVisitCompoundStmt(cast<CompoundStmt>(fDecl->getBody()));
    //End of the function, so we clear all variablesEncountered so far
  currentVarsEncountered.clear();
  return true;
}

bool ASTHeapifyVisitor::subVisitCompoundStmt(CompoundStmt* coSt)
{
  bool deleteEnd=true; //If we have to delete at the end of the scope (so if there is no return)
  // bool curState=variableHeap.found; //If variable is found, we're in a subLoop, so no delete (unless there is a return) 
  std::vector<item_found> currentVarsInScope;
  for (CompoundStmt::body_iterator b = coSt->body_begin(), e = coSt->body_end(); b != e; ++b)
  {
    Stmt* st=*b;
    if(isa<ReturnStmt>(st))
    {
      deleteSegmentAtStmt(*st);
      deleteEnd=false;  //No need to delete at the end of the scope if there is a return (and so a delete)
    }
    else if (isa<GotoStmt>(st))
    {
      deleteSegmentAtStmt(*st);
      deleteEnd=false;
    }
    else if (isa<DeclStmt>(st))
      handleDeclStmt(cast<DeclStmt>(st),currentVarsInScope);
    else if (isa<CompoundStmt>(st))
      subVisitCompoundStmt(cast<CompoundStmt>(st));
    //Will create a scope possibly outside of a compound stmt
    else if (isa<IfStmt>(st))
      subVisitIfStmt(cast<IfStmt>(st));
    else if (isa<ForStmt>(st))
      subVisitForStmt(cast<ForStmt>(st));
    else if (isa<WhileStmt>(st))
      subVisitWhileStmt(cast<WhileStmt>(st));
  }
  //True when we haven't seen any Goto or Return so far.
  //So we have to delete at the end of the scope
  if(deleteEnd)
      TheRewriter.InsertTextAfter(coSt->getEndLoc(),createDeleteSegment(currentVarsInScope)); 
  //We remove variables declared in the scope at the end of it (since they no longer exist)
  for(long unsigned int i=0;i<currentVarsInScope.size();i++)
    currentVarsEncountered.pop_back();
  return true;
}
void ASTHeapifyVisitor::subVisitIfStmt(IfStmt* ifSt)
{  
  std::vector<struct item_found> currentVarsInScope;
  //When there is a declaration in the If, we add a section from its beginning to its end (after the else)
  // In this section we declare its variables on the heap
  if(ifSt->hasVarStorage()||ifSt->hasInitStorage())
  {
    
    std::stringstream SSprint;
    SSprint<<"{\n";
    if(ifSt->getInit()!=NULL&& isa<DeclStmt>(ifSt->getInit()))
    {
      DeclStmt* ifStInit=cast<DeclStmt>(ifSt->getInit());
      SSprint<<stringDeclStmt(ifStInit,currentVarsInScope);
      TheRewriter.RemoveText(SourceRange(ifStInit->getBeginLoc(),ifStInit->getEndLoc()));
    }
    if(ifSt->getConditionVariableDeclStmt()!=NULL)
    {
      DeclStmt* ifStCond=ifSt->getConditionVariableDeclStmt();
      SSprint<<stringDeclStmt(ifStCond,currentVarsInScope);
      if(isa<VarDecl>(ifStCond->getSingleDecl()))
        TheRewriter.ReplaceText(SourceRange(ifStCond->getBeginLoc(),ifStCond->getEndLoc())
        ,(cast<VarDecl>(ifStCond->getSingleDecl()))->getNameAsString());
    }
    //Adds the built text section before the If statement
    TheRewriter.InsertTextAfter(ifSt->getBeginLoc(),SSprint.str());
    //We remove variables seen in the scope at the end of it
    //First declStmt contains variables,
    //add them to curVarEncoutered and curVarsInScope
    //UNHANDLED, either do it in the if BUT c++17 will be required
    //Or outside of if, but then the same has to be done for else if
  }
  handleSubStmt(ifSt->getThen());
  handleSubStmt(ifSt->getElse());
  //True when there are variables to delete at the end
  if(!currentVarsInScope.empty())
    deleteSectionAfterCreatedScope(ifSt->getEndLoc(),currentVarsInScope);
}
void ASTHeapifyVisitor::subVisitForStmt(ForStmt* forSt)
{
  std::vector<struct item_found> currentVarsInScope;
  if(forSt->getInit()&&isa<DeclStmt>(forSt->getInit()))
  {
    std::stringstream SSprint;
    SSprint<<"{\n";
    DeclStmt* forStCond=cast<DeclStmt>(forSt->getInit());
    SSprint<<stringDeclStmt(forStCond,currentVarsInScope);
    TheRewriter.ReplaceText(SourceRange(forStCond->getBeginLoc(),forStCond->getEndLoc()),";");
    //We remove variables seen in the scope at the end of it
    TheRewriter.InsertTextAfter(forSt->getBeginLoc(),SSprint.str());
  }
  handleSubStmt(forSt->getBody());
  if(!currentVarsInScope.empty())
    deleteSectionAfterCreatedScope(forSt->getEndLoc(),currentVarsInScope);
}
//Adds a delete section in a generated scope (used when a section had to be created to handle variable declarations
//in If statement or similar types of statements)
void ASTHeapifyVisitor::deleteSectionAfterCreatedScope(const SourceLocation& deleteLoc,const std::vector<struct item_found>& currentVarsInScope)
{
  std::stringstream SSprint;
  SSprint<<";\n"<<createDeleteSegment(currentVarsInScope)<<"}";
  TheRewriter.InsertTextAfterToken(deleteLoc,SSprint.str()); 
  for(long unsigned int i=0;i<currentVarsInScope.size();i++)
      currentVarsEncountered.pop_back();
}
void ASTHeapifyVisitor::subVisitWhileStmt(WhileStmt* whileSt)
{
  std::vector<struct item_found> currentVarsInScope;
  if(whileSt->hasVarStorage())
  {
    
    std::stringstream SSprint;
    SSprint<<"{\n";
    DeclStmt* whileStCond=whileSt->getConditionVariableDeclStmt();
    SSprint<<stringDeclStmt(whileStCond,currentVarsInScope);
    if(isa<VarDecl>(whileStCond->getSingleDecl()))
      TheRewriter.ReplaceText(SourceRange(whileStCond->getBeginLoc(),whileStCond->getEndLoc())
      ,(cast<VarDecl>(whileStCond->getSingleDecl()))->getNameAsString());
    //We remove variables seen in the scope at the end of it
    TheRewriter.InsertTextAfter(whileSt->getBeginLoc(),SSprint.str());
  }
  handleSubStmt(whileSt->getBody());
  if(!currentVarsInScope.empty())
    deleteSectionAfterCreatedScope(whileSt->getEndLoc(),currentVarsInScope);
}
void ASTHeapifyVisitor::handleSubStmt(Stmt* st)
{
  if(st==NULL)
    ;
  else if(isa<CompoundStmt>(st))
    subVisitCompoundStmt(cast<CompoundStmt>(st));
  else if (isa<IfStmt>(st))
    subVisitIfStmt(cast<IfStmt>(st));
  else if(isa<ForStmt>(st))
    subVisitForStmt(cast<ForStmt>(st));
  else if (isa<WhileStmt>(st))
      subVisitWhileStmt(cast<WhileStmt>(st));
  //Case for single instruction for : for,while or if
  else if (isa<DeclStmt>(st))
  {
    //We treat it as if it was a compound statement
    //We simply add the brackets around it
    std::vector<struct item_found> currentVarsInScope;
    TheRewriter.InsertTextAfter(st->getBeginLoc(),"{");
    handleDeclStmt(cast<DeclStmt>(st),currentVarsInScope);
    TheRewriter.InsertTextAfterToken(st->getEndLoc(),createDeleteSegment(currentVarsInScope)); 
    //We remove variables seen in the scope at the end of it
    for(long unsigned int i=0;i<currentVarsInScope.size();i++)
      currentVarsEncountered.pop_back();
    TheRewriter.InsertTextAfterToken(st->getEndLoc(),"}");
  }
  else if(isa<ReturnStmt>(st))
    deleteSegmentAtStmt(*st);
}
void ASTHeapifyVisitor::handleDeclStmt(DeclStmt* st,std::vector<struct item_found>& currentVarsInScope){
  DeclStmt* decStmt=cast<DeclStmt>(st);
  DeclGroupRef decGrpRef=decStmt->getDeclGroup();
  std::stringstream SSprint;
  for(DeclGroupRef::iterator curDeclPtr=decGrpRef.begin(),decGrpEnd=decGrpRef.end()
  ;curDeclPtr!=decGrpEnd;curDeclPtr++)
  {
    Decl* curDecl=*curDeclPtr;
    if(curDecl!=NULL && isa<VarDecl>(curDecl))
      SSprint<<subVisitVarDecl(*(cast<VarDecl>(curDecl)),currentVarsInScope);
  }
  TheRewriter.ReplaceText(SourceRange(decStmt->getBeginLoc(),decStmt->getEndLoc()),SSprint.str());
  //True when variable was found in the current scope, so we have to add a delete at the end of it
  /*
  if(curState!=variableHeap.found)
  {
    curState=!curState;
    deleteEnd=true;
  }
  */
}

std::string ASTHeapifyVisitor::stringDeclStmt(DeclStmt* st,std::vector<struct item_found>& currentVarsInScope)
{
  DeclStmt* decStmt=cast<DeclStmt>(st);
  DeclGroupRef decGrpRef=decStmt->getDeclGroup();
  std::stringstream SSprint;
  for(DeclGroupRef::iterator curDeclPtr=decGrpRef.begin(),decGrpEnd=decGrpRef.end()
  ;curDeclPtr!=decGrpEnd;curDeclPtr++)
  {
    Decl* curDecl=*curDeclPtr;
    if(curDecl!=NULL && isa<VarDecl>(curDecl))
      SSprint<<subVisitVarDecl(*(cast<VarDecl>(curDecl)),currentVarsInScope);
  }
  return SSprint.str();
}

std::string ASTHeapifyVisitor::subVisitVarDecl(VarDecl& v,std::vector<item_found>& currentVarsInScope)
{     
  //True when VarDecl corresponds to the searched variable
  std::string strRes;
  if(foundCorrectVariable(v,variableHeap.name)&&!isInitNew(v))
  {
    struct item_found curVar;
    initItem(curVar,v);

    //We add it to the variables in the scope and the ones encountered so far
    currentVarsInScope.push_back(curVar);
    currentVarsEncountered.push_back(curVar);
    //TOTEST
     strRes=createCreationString(curVar,TheRewriter.getLangOpts());
  }
  else
    strRes=getCompleteVarDeclStr(v);
  return strRes;
}
bool ASTHeapifyVisitor::deleteSegmentAtStmt(Stmt& st)
{
    //We have to delete the variable when it has been found (and thus created)
    TheRewriter.InsertText(st.getBeginLoc(),createDeleteSegment(currentVarsEncountered));
    return true;
}
void ASTHeapifyVisitor::initItem(struct item_found& item,VarDecl& vDec)
{
  item.name=vDec.getNameAsString();
    item.uid=varCounter[vDec.getNameAsString()];
    item.array=isArrayVariable(vDec);
    item.found=true;
    varCounter[vDec.getNameAsString()]++;
    item.declaration=&vDec;
    item.qTypeNew=vDec.getType();
    if(vDec.getType().getTypePtrOrNull()->isReferenceType()||
    !isInitAReference(vDec))
      item.qTypeNew=getUnreferencedQType(item.qTypeNew,vDec.getASTContext()); 
    if(item.array)
    {
      
      item.qTypeTempMem=vDec.getASTContext().getPointerType(
        vDec.getType().getTypePtrOrNull()->getAsArrayTypeUnsafe()->getElementType());
      item.qTypeVar=getReferenceToQType(item.qTypeTempMem,vDec.getASTContext());
    }
    else
    {
      
      item.qTypeTempMem=vDec.getASTContext().getPointerType(item.qTypeNew);
      item.qTypeTempMem.addConst();
      item.qTypeVar=getReferenceToQType(item.qTypeNew,vDec.getASTContext());
    }  

}