#include "ASTTaskGraphVisitor.hpp"

bool ASTTaskGraphVisitor::VisitFunctionDecl(FunctionDecl *f) {
  if(!TheRewriter.getSourceMgr().isWrittenInMainFile(f->getBeginLoc())) 
    return true;
  PotTaskGraph graph;  
  taskGraphs.push(graph);
  return true;
}

bool ASTTaskGraphVisitor::VisitVarDecl(VarDecl *v) {
  if(!TheRewriter.getSourceMgr().isWrittenInMainFile(v->getBeginLoc())) 
    return true;
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
  return true;
}

bool ASTTaskGraphVisitor::VisitUnaryOperator(UnaryOperator* uop)
{
    if(!TheRewriter.getSourceMgr().isWrittenInMainFile(uop->getBeginLoc())) 
        return true;
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
    return true;
}

bool ASTTaskGraphVisitor::VisitBinaryOperator(BinaryOperator* bop)
{
    if(!TheRewriter.getSourceMgr().isWrittenInMainFile(bop->getBeginLoc())) 
        return true;
    //bop->dump();
    return true;
}

