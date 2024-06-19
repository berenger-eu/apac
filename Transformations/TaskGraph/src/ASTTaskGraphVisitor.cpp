#include "ASTTaskGraphVisitor.hpp"

void ASTTaskGraphVisitor::computeAliasesForRHS(const Expr* expression,std::unordered_set<const VarDecl*>& aliases, Instruction& instr)
{
  int depth;
  const Expr* rhs=expression->IgnoreParenImpCasts();
  const DeclRefExpr* d=getSingleDeclRefExprInsideExpr(rhs);        
  if(d)
  {
    //TODO:Modif ici pour ajout alias aux nodes
    const VarDecl* v=cast<VarDecl>(d->getDecl());
    aliases.insert(v);
    depth=getPtrDepthAccess(*v,*rhs);
    aliasTable.getModifiedVariables(aliases,depth+1);
    for(auto& ali:aliases)
      if(v!=ali)
        instr.curAliases.insert({v,ali});
  }
  //Handle CallExpr ( int * p=min(&a,&b) , p might point to a or b or something new)
  else if(isa<CallExpr>(rhs))
  {
    const CallExpr* c=cast<CallExpr>(rhs);
    const FunctionDecl* f=c->getDirectCallee();
    for(unsigned int i=0;i<c->getNumArgs();i++)
    {
      const ParmVarDecl* curParam=f->getParamDecl(i);
      if( (isPointerQualType(curParam->getType())||isReferenceQualType(curParam->getType())) 
        && !isFullConstType(curParam->getType()))
      {
        const Expr* arg=c->getArg(i);
        std::unordered_set<const VarDecl*> subAliases;
        computeAliasesForRHS(arg,subAliases,instr);
        for(auto& v:subAliases)
          aliases.insert(v);
      }
    }
    //TODO: Handle CallExpr
  }
  else
    handleStmt(*rhs,instr);
}

void ASTTaskGraphVisitor::handleCXXOperatorCallExpr(const CXXOperatorCallExpr& c,Instruction& instr)
{
  if(c.isAssignmentOp()&&c.getNumArgs()==2)
    if(isa<DeclRefExpr>(c.getArg(0))&&isReferenceQualType(c.getArg(0)->getType()))
    {
      const VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(c.getArg(0))->getDecl());
      const DeclRefExpr* d;
      if((d=getSingleDeclRefExprInsideExpr(c.getArg(1)))!=nullptr )
      {
        const VarDecl* v2=cast<VarDecl>(d->getDecl());
        if(v2)
        {
          aliasTable.addAliasReference(v2,v);
          addDependencyRead(instr,v2);
          addDependencyWrite(instr,v);
        }
      }
    }
}

void ASTTaskGraphVisitor::handleUnaryOperator(const UnaryOperator& uop,Instruction& curInstr)
{
    Expr* subExpr=uop.getSubExpr();
    //If we have a variable
    const DeclRefExpr* d;
    if((d=getSingleDeclRefExprInsideExpr(subExpr))!=nullptr)
    {
    //and we increment or decrement it, then it's a read and a write 
      int depth;
      if(uop.isIncrementOp()||uop.isDecrementOp())
      {
        std::unordered_set<const VarDecl*> setVarDecl;
        setVarDecl.insert(cast<VarDecl>(cast<DeclRefExpr>(d)->getDecl()));
        aliasTable.getModifiedVariables(setVarDecl,getPtrDepthAccess(**setVarDecl.begin(),*subExpr));
        for(auto& v:setVarDecl)
        {
          v->dump();
          addDependencyRead(curInstr,v);
          addDependencyWrite(curInstr,v);
        }
        addDependencyRead(curInstr,cast<VarDecl>(cast<DeclRefExpr>(d)->getDecl()));

      }
      //If we access the pointer , then we read the variables it may point to
      else if((depth=getPtrDepthAccess(*cast<VarDecl>(d->getDecl()),uop))>0)
      {
        std::unordered_set<const VarDecl*> setVarDecl;
        const VarDecl* v=cast<VarDecl>(d->getDecl());
        setVarDecl.insert(v);
        int depth=getPtrDepthAccess(*v,uop);
        aliasTable.getModifiedVariables(setVarDecl,depth);
        for(auto& curV:setVarDecl)
          addDependencyRead(curInstr,curV);
        addDependencyRead(curInstr,v);

      }
      //If it's not the address of the variable that is read, then it's a read of the variable
      else if(depth!=-1)
        addDependencyRead(curInstr,cast<VarDecl>(d->getDecl()));        
      //TODO: check if other cases are read and/or write
    }
    //Otherwise, unary expression affects a temporary value so we ignore it but still look through the expression
    else
      handleStmt(*subExpr,curInstr);
}

void ASTTaskGraphVisitor::handleBinaryOperator(const BinaryOperator& bop,Instruction& curInstr)
{
  //Special case for assignment operators, because it is a write
    if(bop.isAssignmentOp())
    {
        //Most likely unnecessary since left side has to be a lvalue because of the assignment operator
      const DeclRefExpr* d=getSingleDeclRefExprInsideExpr(bop.getLHS());
      if(d)
      {
        
        std::unordered_set<const VarDecl*> setLeftVars;
        setLeftVars.insert(cast<VarDecl>(d->getDecl()));
        int depth=getPtrDepthAccess(**setLeftVars.begin(),*bop.getLHS());
        aliasTable.getModifiedVariables(setLeftVars,depth);
        if(isPointerQualType(bop.getLHS()->getType()))
        {
          if(setLeftVars.size()==1)
            aliasTable.removeDependencyPtr(*setLeftVars.begin());

          std::unordered_set<const VarDecl*> aliasesRHS;
          computeAliasesForRHS(bop.getRHS(),aliasesRHS,curInstr);
          
          for(auto& ptrV:setLeftVars)
            for(auto& alias:aliasesRHS)
              aliasTable.addAliasPtr(alias,ptrV);
        }
        for(auto& v:setLeftVars)
        {
          addDependencyWrite(curInstr,v);
          if(isa<CompoundAssignOperator>(bop))
            addDependencyRead(curInstr,v);
        }
        if(depth>0)
        addDependencyRead(curInstr,cast<VarDecl>(d->getDecl()));
        
      }
      else
        handleStmt(*bop.getLHS(),curInstr);
      handleStmt(*bop.getRHS(),curInstr);
    }
    //Otherwise, we just look through the expression on both sides
    else
    {
      handleStmt(*bop.getLHS(),curInstr);
      handleStmt(*bop.getRHS(),curInstr);
    }
}
void ASTTaskGraphVisitor::handleMemberCallExpr(const CXXMemberCallExpr& c,Instruction& curInstr)
{
  Expr* obj=c.getImplicitObjectArgument();
  if(isa<DeclRefExpr>(obj))
  {
    VarDecl* v=cast<VarDecl>(cast<DeclRefExpr>(obj)->getDecl());
    addDependencyRead(curInstr,v);
    if(!c.getMethodDecl()->isConst())
      addDependencyWrite(curInstr,v);
  }
  else
    handleStmt(*obj,curInstr);
  handleCallExpr(c,curInstr);
}
void ASTTaskGraphVisitor::handleCallExpr(const CallExpr& c,Instruction& curInstr)
{
  //TODO: Methods and read/write on object/data ?
  const FunctionDecl& f=*(c.getDirectCallee());
  //We look though each parameter of the function
  for(unsigned int i=0;i<f.getNumParams();i++)
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
        addDependencyRead(curInstr,v);
        addDependencyWrite(curInstr,v);
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
        addDependencyRead(curInstr,v);
        addDependencyWrite(curInstr,v);
        std::unordered_set<const VarDecl*> aliases(aliasTable.getAliased(v));
        for(auto& ali:aliases)
        {
          addDependencyRead(curInstr,ali);
          addDependencyWrite(curInstr,ali);
        }
      }
      else{
        llvm::errs()<<"Failed to find DeclRefExpr\n";
      }
    }
    //Otherwise, we look through the expression since it is the same as looking through any expression
    else
      handleStmt(*b,curInstr);   
  }
}
void ASTTaskGraphVisitor::handleStmt(const Stmt& st,Instruction& instr)
{
  // Simple switch case to call the respective handle method, except for DeclRefExpr which is a variable so it is a read
  if(isa<ReturnStmt>(st))
  {
    if(cast<ReturnStmt>(st).getRetValue())
      handleStmt(*(cast<ReturnStmt>(st).getRetValue()),instr);
  }
  else if(isa<Expr>(st))
  {
    const Expr& curExp=*(cast<Expr>(st).IgnoreParenImpCasts()); 
    if(isa<CXXOperatorCallExpr>(curExp))
    {
      handleCXXOperatorCallExpr(cast<CXXOperatorCallExpr>(curExp),instr);
    }
    else if(isa<UnaryOperator>(curExp))
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
      addDependencyRead(instr,v);
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
      curExp.dump();
    }
  }
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

bool ASTTaskGraphVisitor::TraverseForStmt(ForStmt* f)
{
  if(isInHeaders(TheRewriter.getSourceMgr(),f->getBeginLoc())) 
    return true;
  bool res=true;
  Instruction compInstr{f,getStmtAsString(f,TheRewriter.getLangOpts()),true,0};
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
  Instruction compInstr{i,getStmtAsString(i,TheRewriter.getLangOpts()),true,0};
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