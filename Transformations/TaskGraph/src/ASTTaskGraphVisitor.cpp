#include "ASTTaskGraphVisitor.hpp"

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

void ASTTaskGraphVisitor::subVisitCompoundStmt(CompoundStmt* coSt)
{
  for (auto&b : coSt->body() )
      handleSubStmt(b);
}
void ASTTaskGraphVisitor::handleSubStmt(Stmt* st)
{
    std::vector<Expr*> exprList;
    if(st==NULL)
        ;
    else if(isa<BinaryOperator>(st)){
        subVisitBinaryOperator(cast<BinaryOperator>(st));
    }
    else if(isa<UnaryOperator>(st)){
        subVisitUnaryOperator(cast<UnaryOperator>(st));
    }
    /*   
    else if(isa<DeclStmt>(st)){
        subVisitDeclStmt(cast<DeclStmt>(st));
    }
    
    else if(isa<CallExpr>(st)){
        subVisitCallExpr(cast<CallExpr>(st));
    }
    */
    else{
        llvm::errs()<<"Statement is not handled\n";
    }
    
}


void ASTTaskGraphVisitor::subVisitVarDecl(VarDecl *v) {
  if(v->getInit())
  {
    PotTaskGraph graph=taskGraphs.top();
    PotTask task(0);
    task.addParam(AccessType::AccessWrite, v->getNameAsString());
    std::vector< Stmt*> leafs;
    getLeafs(v->getInit(),leafs); 
    for(auto& b : leafs)
    {
        b->dump();
      if(isa<DeclRefExpr>(b))
      {
        DeclRefExpr& d=cast<DeclRefExpr>(*b);
        task.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      }
    }
   graph.addTask(task);
  }
}

void ASTTaskGraphVisitor::subVisitUnaryOperator(UnaryOperator* uop)
{
    //uop->dump();
    Expr* subExpr=uop->getSubExpr();
    if(isa<DeclRefExpr>(subExpr))
    {
        DeclRefExpr& d=cast<DeclRefExpr>(*subExpr);
        PotTaskGraph graph=taskGraphs.top();
        PotTask task(0);
        task.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
        task.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
        graph.addTask(task);
    }
    //TODO? : Cases like so : f(a)++; with int& f (int& a) {return a;} 
    //Should not happen since function calls are unstacked, so f(a)++ would be temp_var++;    
    else
    {
      llvm::errs()<<"UnaryOperator not handled for following Stmt\n";
      uop->dump();
    }
}

void ASTTaskGraphVisitor::subVisitBinaryOperator(BinaryOperator* bop)
{
    //bop->dump();
    Expr* leftSide=bop->getLHS();
    if(isa<DeclRefExpr>(leftSide))
    {
      DeclRefExpr& d=cast<DeclRefExpr>(*leftSide);
      PotTaskGraph graph=taskGraphs.top();
      PotTask task(0);
      task.addParam(AccessType::AccessWrite, d.getDecl()->getNameAsString());
      //When we have i '+=','-=',... , i is also read
      if(isa<CompoundAssignOperator>(bop))
        task.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
      std::vector< Stmt*> leafs;
      getLeafs(bop->getRHS(),leafs);
      for (auto& b : leafs)
      {
        if(isa<DeclRefExpr>(b))
        {
          DeclRefExpr& d=cast<DeclRefExpr>(*b);
          task.addParam(AccessType::AccessRead, d.getDecl()->getNameAsString());
        }
      }
      graph.addTask(task);
    }
    else
    {
      llvm::errs()<<"BinaryOperator not handled for following Stmt\n";
      bop->dump();
    }
}

