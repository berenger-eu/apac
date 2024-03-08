#include "../include/ASTHeapifyVisitor.hpp"
using namespace clang;

//TODO: Put in a class
struct item_found variableHeap;
struct item_found functionHeap;
std::unordered_map<std::string,int> varCounter;
std::vector<struct item_found> currentVarsEncountered; //TODO implement in cleaner manner

//To see if the function was found (mostly for debugging)
bool visitedOnce=false;
bool ASTHeapifyVisitor::VisitFunctionDecl(FunctionDecl* fDecl)
{
    
    functionHeap.found=functionHeap.found||
    fDecl->getNameAsString().compare(functionHeap.name)==0;
    if(foundCorrectFunction(*fDecl))
      visitedOnce=false;
    
    return true;
}
//Visits each scope, should only visit top scope

bool ASTHeapifyVisitor::VisitCompoundStmt(CompoundStmt* coSt)
{
  if(!visitedOnce){
    visitedOnce=true;
    subVisitCompoundStmt(coSt);
    currentVarsEncountered.clear();
  }
  return true;
}

bool ASTHeapifyVisitor::subVisitCompoundStmt(CompoundStmt* coSt)
{
  bool deleteEnd=true; //If we have to delete at the end of the scope (so if there is no return)
  bool curState=variableHeap.found; //If variable is found, we're in a subLoop, so no delete (unless there is a return) 
  std::vector<item_found> currentVarsInScope;
  for (CompoundStmt::body_iterator b = coSt->body_begin(), e = coSt->body_end(); b != e; ++b)
  {
    Stmt* st=*b;
    if(isa<ReturnStmt>(st))
    {
      ReturnStmt* retStmt=cast<ReturnStmt>(st);
      subVisitReturnStmt(*retStmt);
      deleteEnd=false;  //No need to delete at the end of the scope if there is a return (and so a delete)
    }
    else if (isa<DeclStmt>(st))
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
    else if (isa<CompoundStmt>(st))
      subVisitCompoundStmt(cast<CompoundStmt>(st));
    //Will create a scope possibly outside of a compound stmt
    else if (isa<IfStmt>(st))
      subVisitIfStmt(cast<IfStmt>(st));
  }
  if(deleteEnd)
      TheRewriter.InsertTextAfter(coSt->getEndLoc(),createDeleteSegment(currentVarsInScope)); 
  //We remove variables seen in the scope at the end of it
  for(int i=0;i<currentVarsInScope.size();i++)
    currentVarsEncountered.pop_back();
  return true;
}
void ASTHeapifyVisitor::subVisitIfStmt(IfStmt* ifSt)
{
  
  if(ifSt->hasVarStorage())
  {
    //First declStmt contains variables,
    //add them to curVarEncoutered and curVarsInScope
    //UNHANDLED, either do it in the if BUT c++17 will be required
    //Or outside of if, but then the same has to be done for else if
  }
  handleIfSubStmt(ifSt->getThen());
  handleIfSubStmt(ifSt->getElse());
}
void ASTHeapifyVisitor::handleIfSubStmt(Stmt* st)
{
  if(st==NULL)
    ;
  else if(isa<CompoundStmt>(st))
    subVisitCompoundStmt(cast<CompoundStmt>(st));
  else if (isa<IfStmt>(st))
    subVisitIfStmt(cast<IfStmt>(st));
  //TODO: handle single instructions if
  else if (isa<DeclStmt>(st))
    ;
}
std::string ASTHeapifyVisitor::subVisitVarDecl(VarDecl& v,std::vector<item_found>& currentVarsInScope)
{     
  //True when VarDecl corresponds to the searched variable
  std::string strRes;
  if(foundCorrectVariable(v)&&!isInitNew(v))
  {
    struct item_found curVar;
    initItem(curVar,v);

    //We add it to the variables in the scope and the ones encountered so far
    currentVarsInScope.push_back(curVar);
    currentVarsEncountered.push_back(curVar);
    //TOTEST
     
    //TheRewriter.ReplaceText(SourceRange(v.getTypeSpecStartLoc(),v.getTypeSpecEndLoc()),curVar.qTypeNew.getAsString());
    strRes=createCreationString(curVar);
  }
  else
    strRes=getCompleteVarDeclStr(v);
  return strRes;
}
bool ASTHeapifyVisitor::subVisitReturnStmt(ReturnStmt& retStmt)
{
    //We have to delete the variable when it has been found (and thus created)
    TheRewriter.InsertText(retStmt.getBeginLoc(),createDeleteSegment(currentVarsEncountered));
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