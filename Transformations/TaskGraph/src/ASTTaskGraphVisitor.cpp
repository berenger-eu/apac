#include "ASTTaskGraphVisitor.hpp"

bool ASTTaskGraphVisitor::TraverseFunctionDecl(FunctionDecl *f) {
  if(isInHeaders(TheRewriter.getSourceMgr(),f->getBeginLoc())) 
    return true;
  //If function is not in headers and has a body and is a definition, then we traverse it recursively
  //Using traverse we can avoid visiting nodes that we don't need
  if(f->getBody()&&f->isThisDeclarationADefinition()){
    PotTaskGraph graph;  
    taskGraphs.push(graph);
    return RecursiveASTVisitor::TraverseFunctionDecl(f);
  }
  return true;
}

void ASTTaskGraphVisitor::handleUnaryOperator(const UnaryOperator& uop,PotTask& curTask)
{
    Expr* subExpr=uop.getSubExpr();
    //If we have a variable
    if(isa<DeclRefExpr>(subExpr))
    //and we increment or decrement it, then it's a read and a write 
      if(uop.isIncrementOp()||uop.isDecrementOp())
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*subExpr);
        curTask.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
        curTask.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      }
      //TODO: check if other cases are read and/or write
    //Otherwise, unary expression affects a temporary value so we ignore it but still look through the expression
    else
      handleExpr(*subExpr,curTask);
}

void ASTTaskGraphVisitor::handleBinaryOperator(const BinaryOperator& bop,PotTask& curTask)
{
  //Special case for assignment operators, because it is a write
    if(bop.isAssignmentOp())
    {
        //Most likely unnecessary since left side has to be a lvalue because of the assignment operator
      if(isa<DeclRefExpr>(bop.getLHS()))
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*bop.getLHS());
        curTask.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
        //Also is a read if it is a compound assignment
        if(isa<CompoundAssignOperator>(bop))
          curTask.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      }
      else
        handleExpr(*bop.getLHS(),curTask);
      handleExpr(*bop.getRHS(),curTask);
    }
    //Otherwise, we just look through the expression on both sides
    else
    {
      handleExpr(*bop.getLHS(),curTask);
      handleExpr(*bop.getRHS(),curTask);
    }
}

void ASTTaskGraphVisitor::handleCallExpr(const CallExpr& c,PotTask& curTask)
{
  //TODO: Methods and read/write on object/data ?
  const FunctionDecl& f=*(c.getDirectCallee());
  //We look though each parameter of the function
  for(int i=0;i<f.getNumParams();i++)
  {
    const ParmVarDecl& p=*(f.getParamDecl(i));
    const Expr* b=c.getArg(i);
    //If we have a variable, then there might be a write
    if(isa<DeclRefExpr>(b->IgnoreCasts()))
    {
      const DeclRefExpr& d=cast<DeclRefExpr>(*(b->IgnoreCasts()));
      curTask.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      //If the parameter can be modified (parameter is either a reference or a pointer AND it's not completely const)
      //  then there might be a write, so we assume there is one
      if( !(isFullConstType(p.getType())||!(isReferenceQualType(p.getType())||isPointerQualType(p.getType())) ))
        curTask.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
    }
    //Otherwise, we look through the expression since it is the same as looking through any expression
    else
      handleExpr(*b,curTask);
  }
  
}
void ASTTaskGraphVisitor::handleExpr(const Expr& exp,PotTask& task)
{
  // Simple switch case to call the respective handle method, except for DeclRefExpr which is a variable so it is a read
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
    ;//Do nothing
  }
  else
  {
    llvm::errs()<<"Unhandled expression\n";
    exp.dump();
  }
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