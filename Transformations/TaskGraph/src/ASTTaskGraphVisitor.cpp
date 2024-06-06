#include "ASTTaskGraphVisitor.hpp"

bool isInExceptionList(const ParmVarDecl& p)
{
  return p.getType().getAsString().find("std::shared_ptr") != std::string::npos;
}

void addDependency(Instruction& instr,Access a,const NamedDecl* d)
{
  instr.dependencies.emplace(a,d);
  std::unordered_set<const VarDecl*> aliases;
  aliases.insert(d);
  for (auto dep : aliasTable.getAliases(d))
  {
    if(aliases.count(dep)==0)
    {
      instr.dependencies.emplace(a,dep);
      aliases.insert(dep);
    }
  }
}

bool ASTTaskGraphVisitor::TraverseCXXMethodDecl(CXXMethodDecl *m) {
  if(isInHeaders(TheRewriter.getSourceMgr(),m->getBeginLoc())) 
    return true;

  //If function is not in headers and has a body and is a definition, then we traverse it recursively
  //Using traverse we can avoid visiting nodes that we don't need
  if(m->getBody()&&m->isThisDeclarationADefinition()){ 
    functionsInstructionsVector.push_back(std::vector<Instruction>());
    return RecursiveASTVisitor::TraverseCXXMethodDecl(m);
  }
  return true;
}
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
bool ASTTaskGraphVisitor::TraverseCXXOperatorCallExpr(CXXOperatorCallExpr* c)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),c->getBeginLoc())) 
    return true;
  //Most likely is the definition of a reference
  if(c->isAssignmentOp()&&c->getNumArgs()==2)
    if(isa<DeclRefExpr>(c->getArg(0))&&isReferenceQualType(c->getArg(0)->getType()))
    {
      const VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(c->getArg(0))->getDecl());
      const DeclRefExpr* d;
      if((d=getDeclRefExprInsideExpr(c->getArg(1)))!=nullptr )
      {
        const VarDecl* v2=cast<VarDecl>(d->getDecl());
        if(v2)
        {
          aliasTable.addAliasReference(v2,v);
        }
      }
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
        VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(*subExpr).getDecl());
        curInstr.dependencies.emplace(Access::WRITE, v->getCanonicalDecl());
        curInstr.dependencies.emplace(Access::READ, v->getCanonicalDecl());
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
        VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(bop.getLHS())->getDecl());
        curInstr.dependencies.emplace(Access::WRITE, v->getCanonicalDecl());
        //Also is a read if it is a compound assignment
        if(isa<CompoundAssignOperator>(bop))
          curInstr.dependencies.emplace(Access::READ, v->getCanonicalDecl());
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
void ASTTaskGraphVisitor::handleMemberCallExpr(const CXXMemberCallExpr& c,Instruction& curInstr)
{
  Expr* obj=c.getImplicitObjectArgument();
  if(isa<DeclRefExpr>(obj))
  {
    VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(obj)->getDecl());
    curInstr.dependencies.emplace(Access::READ, v->getCanonicalDecl());
    if(!c.getMethodDecl()->isConst())
      curInstr.dependencies.emplace(Access::WRITE, v->getCanonicalDecl());
  }
  else
    handleExpr(*obj,curInstr);
  handleCallExpr(c,curInstr);
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
    if(isInExceptionList(p))
    {
      const Expr* d=b;
      if(isa<CXXBindTemporaryExpr>(d))
      {
        d=cast<CXXBindTemporaryExpr>(*(b->IgnoreCasts())).getSubExpr();
      }
      if(isa<CXXConstructExpr>(d))
        d=cast<CXXConstructExpr>(d)->getArg(0);
      if(isa<DeclRefExpr>(d->IgnoreParenImpCasts()))
      {
        const VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(d->IgnoreParenImpCasts())->getDecl()); 
        curInstr.dependencies.emplace(Access::READ, v->getCanonicalDecl());
        curInstr.dependencies.emplace(Access::WRITE, v->getCanonicalDecl());
      }
    }
    else if( !(isFullConstType(p.getType())&&isReferenceQualType(p.getType()))
      ||isPointerQualType(p.getType()) )
    {
      const Expr* curExpr =b; 
      curExpr=curExpr->IgnoreCasts();
      while(isa<UnaryOperator>(curExpr))
      {
        curExpr=cast<UnaryOperator>(curExpr)->getSubExpr();
        curExpr=curExpr->IgnoreCasts();
      }
      //If the parameter can be modified (parameter is either a reference or a pointer AND it's not completely const)
      //  then there might be a write, so we assume there is one
      if(isa<DeclRefExpr>(curExpr))
      {
        const VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(curExpr)->getDecl());
        curInstr.dependencies.emplace(Access::READ, v->getCanonicalDecl());
        curInstr.dependencies.emplace(Access::WRITE, v->getCanonicalDecl());
      }
      else{
        llvm::errs()<<"Failed to find DeclRefExpr\n";
      }
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
  else if(isa<CXXMemberCallExpr>(curExp))
  {
    handleMemberCallExpr(cast<CXXMemberCallExpr>(curExp),instr);
  }
  else if(isa<CallExpr>(curExp))
  {
    handleCallExpr(cast<CallExpr>(curExp),instr);
  }
  else if(isa<DeclRefExpr>(curExp))
  {
    const VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(curExp).getDecl());
    instr.dependencies.emplace(Access::READ, v->getCanonicalDecl());
  }
  //Ignored expressions case
  else if(isa<IntegerLiteral>(curExp))
  {
    ;//Do nothing
  }
  else
  {
    llvm::errs()<<"Unhandled expression\n";
    llvm::errs()<<TheRewriter.getSourceMgr().getPresumedLoc(curExp.getBeginLoc()).getFilename()<<":";
    exp.dump();
  }
}

bool ASTTaskGraphVisitor::TraverseUnaryOperator(UnaryOperator* uop)
{

  if(isInHeaders(TheRewriter.getSourceMgr(),uop->getBeginLoc())) 
    return true;
  Instruction instr{uop,getStmtAsString(uop,TheRewriter.getLangOpts()),false};
  handleUnaryOperator(*uop,instr);
  functionsInstructionsVector.back().push_back(instr);
  return true;
}

bool ASTTaskGraphVisitor::TraverseBinaryOperator(BinaryOperator* bop)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),bop->getBeginLoc())) 
    return true;
  Instruction instr{bop,getStmtAsString(bop,TheRewriter.getLangOpts()),false};
  handleBinaryOperator(*bop,instr);
  functionsInstructionsVector.back().push_back(instr);  
  return true;
}
bool ASTTaskGraphVisitor::TraverseCompoundAssignOperator(CompoundAssignOperator* bop)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),bop->getBeginLoc())) 
    return true;
  Instruction instr{bop,getStmtAsString(bop,TheRewriter.getLangOpts()),false};
  handleBinaryOperator(*bop,instr);
  functionsInstructionsVector.back().push_back(instr);  
  return true;
}
bool ASTTaskGraphVisitor::TraverseCXXMemberCallExpr(CXXMemberCallExpr* c)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),c->getBeginLoc())) 
    return true;
  c->dump();
  llvm::outs()<<"Method\n";
  Instruction instr{c,getStmtAsString(c,TheRewriter.getLangOpts()),false};
  handleMemberCallExpr(*c,instr); 
  functionsInstructionsVector.back().push_back(instr);  
  return true;
}
bool ASTTaskGraphVisitor::TraverseCallExpr(CallExpr* c)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),c->getBeginLoc())) 
    return true;
  Instruction instr{c,getStmtAsString(c,TheRewriter.getLangOpts()),false};
  handleCallExpr(*c,instr);
  functionsInstructionsVector.back().push_back(instr);  
  return true;
}

bool ASTTaskGraphVisitor::TraverseReturnStmt(ReturnStmt* r)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),r->getBeginLoc())) 
    return true;
  Instruction instr{r,getStmtAsString(r,TheRewriter.getLangOpts()),false};
  if(r->getRetValue())
    handleExpr(*(r->getRetValue()),instr);
  functionsInstructionsVector.back().push_back(instr);
  return true;
}
bool ASTTaskGraphVisitor::TraverseForStmt(ForStmt* f)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),f->getBeginLoc())) 
    return true;
  bool res=true;
  std::stringstream ss;
  ss<<"for("<<getStmtAsString(f->getInit(),TheRewriter.getLangOpts())<<";"
  <<getExprAsString(f->getCond(),TheRewriter.getLangOpts())<<";"
  <<getExprAsString(f->getInc(),TheRewriter.getLangOpts())<<")";
  Instruction compInstr{f,ss.str(),true,0};
  functionsInstructionsVector.push_back(std::vector<Instruction>());
  res=RecursiveASTVisitor::TraverseForStmt(f);
  compInstr.scopedInstructions=functionsInstructionsVector.back();
  for(auto& instr:compInstr.scopedInstructions){
    for(auto& dep:instr.dependencies){
      compInstr.dependencies.insert(dep);
    }
    if(instr.complexInstruction){
      compInstr.scopedInstructionsNumber+=instr.scopedInstructions.size();
    }
    compInstr.scopedInstructionsNumber++;
  }
  functionsInstructionsVector.pop_back();
  functionsInstructionsVector.back().push_back(compInstr);
  
  return res;
}

bool ASTTaskGraphVisitor::TraverseIfStmt(IfStmt* i)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),i->getBeginLoc())) 
    return true;
  bool res =true;
  std::stringstream ss;
  ss<<"if("<<getExprAsString(i->getCond(),TheRewriter.getLangOpts())<<")";
  Instruction compInstr{i,ss.str(),true,0};
  functionsInstructionsVector.push_back(std::vector<Instruction>());
  // res=RecursiveASTVisitor::TraverseIfStmt(i);
  if(i->getThen()&&isa<CompoundStmt>(i->getThen()))
  {
    CompoundStmt* c=cast<CompoundStmt>(i->getThen());
    Instruction compInstr;
    compInstr.instruction=c;
    std::stringstream ss;
    ss<<"if";

    compInstr.instructionString=ss.str();
    compInstr.complexInstruction=true;
    compInstr.scopedInstructionsNumber=0;
    functionsInstructionsVector.push_back(std::vector<Instruction>());
    res=RecursiveASTVisitor::TraverseCompoundStmt(c);
    compInstr.scopedInstructions=functionsInstructionsVector.back();
    for(auto& instr:compInstr.scopedInstructions){
      for(auto& dep:instr.dependencies){
        compInstr.dependencies.insert(dep);
      }
      if(instr.complexInstruction){
        compInstr.scopedInstructionsNumber+=instr.scopedInstructions.size();
      }
      compInstr.scopedInstructionsNumber++;
    }
    functionsInstructionsVector.pop_back();
    functionsInstructionsVector.back().push_back(compInstr);

  }
  if(i->getElse())
  {
    if(isa<CompoundStmt>(i->getElse()))
    {

      CompoundStmt* c=cast<CompoundStmt>(i->getElse());
      Instruction compInstr;
      compInstr.instruction=c;
      std::stringstream ss;
      ss<<"else";

      compInstr.instructionString=ss.str();
      compInstr.complexInstruction=true;
      compInstr.scopedInstructionsNumber=0;

      functionsInstructionsVector.push_back(std::vector<Instruction>());
      res=RecursiveASTVisitor::TraverseCompoundStmt(c);
      compInstr.scopedInstructions=functionsInstructionsVector.back();
      for(auto& instr:compInstr.scopedInstructions){
        for(auto& dep:instr.dependencies){
          compInstr.dependencies.insert(dep);
        }
        if(instr.complexInstruction){
          compInstr.scopedInstructionsNumber+=instr.scopedInstructions.size();
        }
        compInstr.scopedInstructionsNumber++;
      }
      functionsInstructionsVector.pop_back();
      functionsInstructionsVector.back().push_back(compInstr);
    }
    else
    {
      Instruction instr;
      instr.instruction=i->getElse();
      instr.instructionString="else";
      compInstr.complexInstruction=true;
      compInstr.scopedInstructionsNumber=0;
      functionsInstructionsVector.push_back(std::vector<Instruction>());
      res=RecursiveASTVisitor::TraverseStmt(i->getElse());
      compInstr.scopedInstructions=functionsInstructionsVector.back();
      for(auto& instr:compInstr.scopedInstructions){
        for(auto& dep:instr.dependencies){
          compInstr.dependencies.insert(dep);
        }
        if(instr.complexInstruction){
          compInstr.scopedInstructionsNumber+=instr.scopedInstructions.size();
        }
        compInstr.scopedInstructionsNumber++;
      }
      functionsInstructionsVector.pop_back();
      functionsInstructionsVector.back().push_back(compInstr);
    }
  }
  compInstr.scopedInstructions=functionsInstructionsVector.back();
  for(auto& instr:compInstr.scopedInstructions){
    for(auto& dep:instr.dependencies){
      compInstr.dependencies.insert(dep);
    }
    if(instr.complexInstruction){
      compInstr.scopedInstructionsNumber+=instr.scopedInstructions.size();
    }
    compInstr.scopedInstructionsNumber++;
  }
  functionsInstructionsVector.pop_back();
  functionsInstructionsVector.back().push_back(compInstr);
  
  return res;
}