#include "ASTTaskGraphVisitor.hpp"
/*
bool ASTTaskGraphVisitor::VisitFunctionDecl(FunctionDecl *f) {
  if(isInHeaders(TheRewriter.getSourceMgr(),f->getBeginLoc())) 
    return true;
  if(f->getBody()&&f->isThisDeclarationADefinition()){
    PotTaskGraph graph;  
    taskGraphs.push(graph);
    subVisitCompoundStmt(cast<CompoundStmt>(f->getBody()));
  }
  return true;
}
*/
bool ASTTaskGraphVisitor::TraverseFunctionDecl(FunctionDecl *f) {
  if(isInHeaders(TheRewriter.getSourceMgr(),f->getBeginLoc())) 
    return true;
  if(f->getBody()&&f->isThisDeclarationADefinition()){
    PotTaskGraph graph;  
    taskGraphs.push(graph);
    return RecursiveASTVisitor::TraverseFunctionDecl(f);
  }
  return true;
}
bool ASTTaskGraphVisitor::TraverseDeclStmt(DeclStmt* declSt)
{
  return true;
}
bool ASTTaskGraphVisitor::TraverseUnaryOperator(UnaryOperator* uop)
{
  PotTask task(0);
  handleUnaryOperator(*uop,task);
  PotTaskGraph& graph=taskGraphs.top();
  if(!isEmptyTask(task)){
    PotTaskGraph& graph=taskGraphs.top();
    graph.addTask(task);
  }
  return true;
}
bool ASTTaskGraphVisitor::TraverseBinaryOperator(BinaryOperator* bop)
{
  PotTask task(0);
  handleBinaryOperator(*bop,task);
  PotTaskGraph& graph=taskGraphs.top();
  if(!isEmptyTask(task)){
    PotTaskGraph& graph=taskGraphs.top();
    graph.addTask(task);
  }
  return true;
}
bool ASTTaskGraphVisitor::TraverseCallExpr(CallExpr* c)
{
  PotTask task(0);
  handleCallExpr(*c,task);
  if(!isEmptyTask(task)){
    PotTaskGraph& graph=taskGraphs.top();
    graph.addTask(task);
  }
  return true;
}

void ASTTaskGraphVisitor::subVisitVarDecl(VarDecl *v) {
  /*
  if(v->getInit())
  {
    task.addParam(AccessType::AccessWrite, v->getNameAsString());
    std::vector< Stmt*> leafs;
    getLeafs(v->getInit(),leafs); 
    llvm::errs()<<"Leafs size "<<leafs.size()<<"\n";
    for(auto& b : leafs)
    {
        b->dump();
      if(isa<DeclRefExpr>(b))
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*b);
        task.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      }
    }
  }
  */
}

void ASTTaskGraphVisitor::handleUnaryOperator(const UnaryOperator& uop,PotTask& curTask)
{
    Expr* subExpr=uop.getSubExpr();
    if(isa<DeclRefExpr>(subExpr))
      if(uop.isIncrementOp()||uop.isDecrementOp())
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*subExpr);
        curTask.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
        curTask.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      }
    else
      handleExpr(*subExpr,curTask);
}

void ASTTaskGraphVisitor::handleBinaryOperator(const BinaryOperator& bop,PotTask& curTask)
{
  //Special case for assignment operators, because it is a write
    if(bop.isAssignmentOp())
    {
        //Most likely unnecessary since it has to be a lvalue
      if(isa<DeclRefExpr>(bop.getLHS()))
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*bop.getLHS());
        curTask.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
        if(isa<CompoundAssignOperator>(bop))
          curTask.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      }
      else
        handleExpr(bop.getLHS(),curTask);
      handleExpr(bop.getRHS(),curTask);
    }
    else
    {
      handleExpr(bop.getLHS(),curTask);
      handleExpr(bop.getRHS(),curTask);
    }
}

void ASTTaskGraphVisitor::handleCallExpr(const CallExpr& c,PotTask& curTask)
{
  /*
  std::vector< Stmt*> leafs;
  getLeafs(c,leafs);
  for(auto& b : leafs)
  {
    if(isa<DeclRefExpr>(b))
    {
      DeclRefExpr& d=cast<DeclRefExpr>(*b);
      task.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      //TODO: Analyze dependencies 
      //task.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
    }
  }
  */
  return false;
}