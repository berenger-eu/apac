#include "../include/ASTHeapifyVisitor.hpp"
using namespace clang;

bool ASTHeapifyVisitor::VisitStmt(Stmt *s)
{
    return true;
}
struct item_found variableHeap;
struct item_found functionHeap;
std::unordered_map<std::string,int> varCounter;
std::vector<struct item_found> currentVarsEncountered; //TODO implement in cleaner manner

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
bool visitedOnce=false;
bool ASTHeapifyVisitor::VisitCompoundStmt(CompoundStmt* coSt)
{
  if(!visitedOnce){
    visitedOnce=true;
    subVisitCompoundStmt(coSt);
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
      /*
      if(decStmt->isSingleDecl())
        dec=decStmt->getSingleDecl();
      */std::stringstream SSprint;
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
  }
  if(deleteEnd)
      TheRewriter.InsertTextAfter(coSt->getEndLoc(),createDeleteSegment(currentVarsInScope)); 
  for(int i=0;i<currentVarsInScope.size();i++)
    currentVarsEncountered.pop_back();
  return true;
}

std::string ASTHeapifyVisitor::subVisitVarDecl(VarDecl& v,std::vector<item_found>& currentVarsInScope)
{     
  //True when VarDecl corresponds to the searched variable
  std::string strRes;
  if(foundCorrectVariable(v)&&!isInitNew(v))
  {
    struct item_found curVar;
    curVar.name=v.getNameAsString();
    curVar.uid=varCounter[v.getNameAsString()];
    curVar.array=isArrayVariable(v);
    curVar.found=true;
    varCounter[v.getNameAsString()]++;
    curVar.declaration=&v;
    curVar.qTypeNew=v.getType();
    if(v.getType().getTypePtrOrNull()->isReferenceType()||
    isConstantInit(v))
      curVar.qTypeNew=unreferenceQType(curVar.qTypeNew,v.getASTContext()); 
    if(curVar.array)
    {
      
      curVar.qTypeTempMem=v.getASTContext().getPointerType(v.getType().getTypePtrOrNull()->getAsArrayTypeUnsafe()->getElementType());
      curVar.qTypeVar=referenceToQType(curVar.qTypeTempMem,v.getASTContext());
    }
    else
    {
      
      curVar.qTypeTempMem=v.getASTContext().getPointerType(curVar.qTypeNew);
      curVar.qTypeTempMem.addConst();
      curVar.qTypeVar=referenceToQType(curVar.qTypeNew,v.getASTContext());
    }  

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
