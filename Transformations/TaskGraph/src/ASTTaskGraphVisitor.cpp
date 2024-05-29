#include "ASTTaskGraphVisitor.hpp"

bool ASTTaskGraphVisitor::TraverseFunctionDecl(FunctionDecl *f) {
  if(isInHeaders(TheRewriter.getSourceMgr(),f->getBeginLoc())) 
    return true;
  //If function is not in headers and has a body and is a definition, then we traverse it recursively
  //Using traverse we can avoid visiting nodes that we don't need
  if(f->getBody()&&f->isThisDeclarationADefinition()){ 
    functionsInstructionsVector.push_back(std::vector<Instruction>());
    return RecursiveASTVisitor::TraverseFunctionDecl(f);
  }
  return true;
}

void ASTTaskGraphVisitor::handleUnaryOperator(const UnaryOperator& uop,Instruction& curInstr)
{
    Expr* subExpr=uop.getSubExpr();
    //If we have a variable
    if(isa<DeclRefExpr>(subExpr))
    //and we increment or decrement it, then it's a read and a write 
      if(uop.isIncrementOp()||uop.isDecrementOp())
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*subExpr);
        curInstr.dependencies.emplace(Access::WRITE, d.getDecl()->getCanonicalDecl());
        curInstr.dependencies.emplace(Access::READ, d.getDecl()->getCanonicalDecl());
      }
      //TODO: check if other cases are read and/or write
    //Otherwise, unary expression affects a temporary value so we ignore it but still look through the expression
    else
      handleExpr(*subExpr,curInstr);
}

void ASTTaskGraphVisitor::handleBinaryOperator(const BinaryOperator& bop,Instruction& curInstr)
{
  //Special case for assignment operators, because it is a write
    if(bop.isAssignmentOp())
    {
        //Most likely unnecessary since left side has to be a lvalue because of the assignment operator
      if(isa<DeclRefExpr>(bop.getLHS()))
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*bop.getLHS());
        curInstr.dependencies.emplace(Access::WRITE, d.getDecl()->getCanonicalDecl());
        //Also is a read if it is a compound assignment
        if(isa<CompoundAssignOperator>(bop))
          curInstr.dependencies.emplace(Access::READ, d.getDecl()->getCanonicalDecl());
      }
      else
        handleExpr(*bop.getLHS(),curInstr);
      handleExpr(*bop.getRHS(),curInstr);
    }
    //Otherwise, we just look through the expression on both sides
    else
    {
      handleExpr(*bop.getLHS(),curInstr);
      handleExpr(*bop.getRHS(),curInstr);
    }
}

void ASTTaskGraphVisitor::handleCallExpr(const CallExpr& c,Instruction& curInstr)
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
      curInstr.dependencies.emplace(Access::READ, d.getDecl()->getCanonicalDecl());
      
      //If the parameter can be modified (parameter is either a reference or a pointer AND it's not completely const)
      //  then there might be a write, so we assume there is one
      if( !(isFullConstType(p.getType())||!(isReferenceQualType(p.getType())||isPointerQualType(p.getType())) ))
        curInstr.dependencies.emplace(Access::WRITE, d.getDecl()->getCanonicalDecl());
    }
    //Otherwise, we look through the expression since it is the same as looking through any expression
    else
      handleExpr(*b,curInstr);
  }
  
}
void ASTTaskGraphVisitor::handleExpr(const Expr& exp,Instruction& instr)
{
  // Simple switch case to call the respective handle method, except for DeclRefExpr which is a variable so it is a read
  const Expr& curExp=*exp.IgnoreCasts(); 
  if(isa<UnaryOperator>(curExp))
  {
    handleUnaryOperator(cast<UnaryOperator>(curExp),instr);
  }
  else if(isa<BinaryOperator>(curExp))
  {
    handleBinaryOperator(cast<BinaryOperator>(curExp),instr);
  }
  else if(isa<CallExpr>(curExp))
  {
    handleCallExpr(cast<CallExpr>(curExp),instr);
  }
  else if(isa<DeclRefExpr>(curExp))
  {
    const DeclRefExpr& d=cast<DeclRefExpr>(curExp);
    instr.dependencies.emplace(Access::READ, d.getDecl()->getCanonicalDecl());
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
  Instruction instr;
  instr.instruction=uop;
  instr.instructionString=getStmtAsString(uop,TheRewriter.getLangOpts());
  handleUnaryOperator(*uop,instr);
  std::vector<Instruction>& functionInstructions=functionsInstructionsVector.back();
  functionInstructions.push_back(instr);
  return true;
}

bool ASTTaskGraphVisitor::TraverseBinaryOperator(BinaryOperator* bop)
{
  Instruction instr;
  instr.instruction=bop;
  instr.instructionString=getStmtAsString(bop,TheRewriter.getLangOpts());
  handleBinaryOperator(*bop,instr);
  std::vector<Instruction>& functionInstructions=functionsInstructionsVector.back();
  functionInstructions.push_back(instr);
  return true;
}
bool ASTTaskGraphVisitor::TraverseCallExpr(CallExpr* c)
{
  Instruction instr;
  instr.instruction=c;
  instr.instructionString=getStmtAsString(c,TheRewriter.getLangOpts());
  handleCallExpr(*c,instr);
  std::vector<Instruction>& functionInstructions=functionsInstructionsVector.back();
  functionInstructions.push_back(instr);
  return true;
}