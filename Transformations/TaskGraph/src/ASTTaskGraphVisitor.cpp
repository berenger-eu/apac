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
        handleExpr(*bop.getLHS(),curTask);
      handleExpr(*bop.getRHS(),curTask);
    }
    else
    {
      handleExpr(*bop.getLHS(),curTask);
      handleExpr(*bop.getRHS(),curTask);
    }
}

void ASTTaskGraphVisitor::handleCallExpr(const CallExpr& c,PotTask& curTask)
{
  const FunctionDecl& f=*(c.getDirectCallee());
  for(int i=0;i<f.getNumParams();i++)
  {
    const ParmVarDecl& p=*(f.getParamDecl(i));
    const Expr* b=c.getArg(i);
    if(isa<DeclRefExpr>(b->IgnoreCasts()))
    {
      const DeclRefExpr& d=cast<DeclRefExpr>(*(b->IgnoreCasts()));
      curTask.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      if(isFullConstType(p.getType())||!(isReferenceQualType(p.getType())||isPointerQualType(p.getType())))
        ;
      else
        curTask.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
      //task.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
    }
    else
      handleExpr(*b,curTask);
  }
  
}
void ASTTaskGraphVisitor::handleExpr(const Expr& exp,PotTask& task)
{
  const Expr& curExp=*exp.IgnoreCasts(); 
  if(isa<UnaryOperator>(curExp))
  {
    handleUnaryOperator(cast<UnaryOperator>(curExp),task);
  }
  else if(isa<BinaryOperator>(curExp))
  {
    handleBinaryOperator(cast<BinaryOperator>(curExp),task);
  }
  else if(isa<CallExpr>(curExp))
  {
    handleCallExpr(cast<CallExpr>(curExp),task);
  }
  else if(isa<DeclRefExpr>(curExp))
  {
    const DeclRefExpr& d=cast<DeclRefExpr>(curExp);
    task.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
  }
  //Ignored expressions case
  else if(isa<IntegerLiteral>(curExp))
  {
    //Do nothing
  }
  else
  {
    llvm::errs()<<"Unhandled expression\n";
    exp.dump();
  }
}