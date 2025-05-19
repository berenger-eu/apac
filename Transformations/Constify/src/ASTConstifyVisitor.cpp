#include "ASTConstifyVisitor.hpp"
using namespace clang;

bool ASTConstifyVisitor::VisitCXXMethodDecl(CXXMethodDecl *methDecl) {
  if (TheRewriter.getSourceMgr().isInSystemHeader(methDecl->getBeginLoc())) {
    return true;
  }
  const_arg &methArg = (*SymT.getInnerConstArg(methDecl));
  for (std::unordered_map<Decl *, struct const_arg>::iterator it =
           SymT.const_arg_table.begin();
       it != SymT.const_arg_table.end(); ++it) {
    const_arg &curArg = it->second;
    // Method with same name
    if (curArg.method && curArg.method != methArg.method &&
        curArg.method->getNameAsString() == methDecl->getNameAsString()) {
      // In same context
      if (curArg.method->getDeclContext()->Equals(methDecl->getDeclContext())) {
        // If it is not const in source file, then we unconst it as a safety
        // measure
        if (!methDecl->isConst()) {
          llvm::errs() << "\nMethod " << methDecl->getNameAsString()
                       << " shares its name with another method in the same "
                          "context so it was unconstified\n";
          unconstifyByPropagation(SymT.getInnerConstArg(methDecl));
        }
      }
    }
  }
  return true;
}

bool ASTConstifyVisitor::VisitBinaryOperator(BinaryOperator *bop) {
  if (TheRewriter.getSourceMgr().isInSystemHeader(bop->getBeginLoc())) {
    return true;
  }

  if (bop->isAssignmentOp()) {
    const_arg *leftSideArg = SymT.getInnerConstArg(bop->getLHS());
    // rightSide=cast<DeclRefExpr>(bop->getRHS());
    // Variable on the left side is modified, so we unconst it
    unconstifyByPropagation(leftSideArg);
  }

  return true;
}
bool ASTConstifyVisitor::VisitUnaryOperator(UnaryOperator *uop) {
  if (TheRewriter.getSourceMgr().isInSystemHeader(uop->getBeginLoc())) {
    return true;
  }
  // Unary operators which modify the variable
  if (uop->isIncrementDecrementOp()) {
    const_arg *innerDeclArg = SymT.getInnerConstArg(uop->getSubExpr());
    unconstifyByPropagation(innerDeclArg);
  }

  return true;
}

bool ASTConstifyVisitor::VisitReturnStmt(ReturnStmt *retStmt) {
  if (TheRewriter.getSourceMgr().isInSystemHeader(retStmt->getBeginLoc())) {
    return true;
  }
  Expr *retValue = retStmt->getRetValue();
  // Returning a pointer might lead to modifications, so we have to unconst
  if (retValue != nullptr && isPointerQualType(retValue->getType())) {
    const_arg *retValDeclArg = SymT.getInnerConstArg(retValue);
    unconstifyByPropagation(retValDeclArg);
  }
  // Returning a reference might lead to modifications, so we have to unconst
  else if (retValue != nullptr && isa<DeclRefExpr>(retValue)) {
    // We cast it to its decl because the QualType of retValue for a reference
    // will be int
    ValueDecl *retDecl = cast<DeclRefExpr>(retValue)->getDecl();
    if (isReferenceQualType(retDecl->getType())) {
      const_arg *retValDeclArg = SymT.getInnerConstArg(retValue);
      unconstifyByPropagation(retValDeclArg);
    }
  }
  return true;
}

/*
bool ASTConstifyVisitor::VisitCXXMemberCallExpr(CXXMemberCallExpr* memExpr)
{
    if(TheRewriter.getSourceMgr().isInSystemHeader(memExpr->getBeginLoc()))
        return true;
    llvm::outs()<<"ICI3";
    if(isa<CXXMethodDecl>(memExpr->getMethodDecl()))
    {
        CXXMethodDecl* methDecl=cast<CXXMethodDecl>(memExpr->getMethodDecl());
        if(!methDecl->isConst())
        {
            memExpr->getCallee()->dump();
            //unconstifyByPropagation(getHashTableValue(getInnerPtr(memExpr->getBase())));
        }
    }
    return true;
}
*/
bool ASTConstifyVisitor::VisitCallExpr(CallExpr *ce) {

  if (TheRewriter.getSourceMgr().isInSystemHeader(ce->getBeginLoc())) {
    return true;
  }
  if (isa<CXXMemberCallExpr>(ce)) {
    CXXMemberCallExpr *memCallExpr = cast<CXXMemberCallExpr>(ce);
    CXXMethodDecl *methDecl = memCallExpr->getMethodDecl();
    if (!methDecl->isConst()) {
      const_arg *innerArg = SymT.getInnerConstArg(
          cast<MemberExpr>(memCallExpr->getCallee())->getBase());
      unconstifyByPropagation(innerArg);
      // unconstifyByPropagation(getHashTableValue(getInnerPtr(memExpr->getBase())));
    }
    return true;
  }
  FunctionDecl *fdec = ce->getDirectCallee();
  assert(ce->getDirectCallee() != nullptr);
  if (fdec != nullptr &&
      TheRewriter.getSourceMgr().isInSystemHeader(fdec->getBeginLoc())) {
    for (auto it = fdec->param_begin(); it != fdec->param_end(); ++it) {
      ParmVarDecl *parVar = *it;
      QualType qtPar = parVar->getType();
      // When the parameter might lead to modifications (not constified and a
      // pointer or a reference)
      if (!parVar->getType().isConstQualified() &&
          (isPointerQualType(qtPar) || isReferenceQualType(qtPar))) {
        int index = std::distance(fdec->param_begin(), it);
        const_arg *curDeclArg = SymT.getInnerConstArg(ce->getArg(index));
        unconstifyByPropagation(curDeclArg);
      }
    }
  }
  return true;
}
bool ASTConstifyVisitor::VisitVarDecl(VarDecl *v) {
  if (TheRewriter.getSourceMgr().isInSystemHeader(v->getBeginLoc())) {
    return true;
  }
  // Initialize the const_arg for any variable
  const_arg *curDeclArg = SymT.getInnerConstArg(v);
  const Type *intype = v->getType().getTypePtrOrNull();
  // We unconstify the variable if it's initialized by the return of a function
  if (valueInit(v) && intype != nullptr) {
    if ((intype->isPointerType() || intype->isReferenceType())) {
      if (isExprACall(v->getInit())) {
        unconstifyByPropagation(curDeclArg);
      }
    }
  }
  return true;
}
void unconstifyByPropagation(const_arg *varArg) {
  std::stack<const_arg *> stackUnconst;
  stackUnconst.push(varArg);
  // Will unconst all const dependencies,
  while (!stackUnconst.empty()) {
    const_arg *curArg = stackUnconst.top();
    stackUnconst.pop();
    if (curArg != nullptr) {
      curArg->is_const = false;
      for (std::vector<const_arg *>::iterator it = curArg->dependencies.begin();
           it != curArg->dependencies.end(); ++it) {
        // If it is not const, then it has already been unconstified, so no need
        // to add it to the stack
        if ((*it)->is_const) {
          stackUnconst.push(*it);
        }
      }
    }
  }
}
