#include "ParamWriteAnalyzer.hpp"

void ParamWriteAnalyzer::analyzeTranslationUnit(ASTContext &Ctx) {
  ctx_ = &Ctx;
  writtenParams_.clear();
  functions_.clear();
  TraverseDecl(Ctx.getTranslationUnitDecl());
  // Start optimistically, then monotonically propagate writes through calls.
  // This handles forward declarations and mutually recursive functions without
  // depending on the order of definitions in the source file.
  bool changed;
  do {
    changed = false;
    for (const auto *function : functions_) {
      auto &written = writtenParams_[function->getCanonicalDecl()];
      auto before = written.size();
      analyzeFunction(function);
      changed |= written.size() != before;
    }
  } while (changed);
}

bool ParamWriteAnalyzer::VisitFunctionDecl(FunctionDecl *f) {
  if (f->hasBody() && f->isThisDeclarationADefinition()) {
    // Skip functions defined in headers (standard library, etc.)
    if (ctx_) {
      SourceManager &sm = ctx_->getSourceManager();
      if (isInHeaders(sm, f->getLocation()))
        return true;
    }
    functions_.push_back(f);
    writtenParams_[f->getCanonicalDecl()];
  }
  return true;
}

bool ParamWriteAnalyzer::isParamReadOnly(const FunctionDecl *f, unsigned paramIdx) const {
  if (!f || paramIdx >= f->getNumParams())
    return false;
  const FunctionDecl *canonical = f->getCanonicalDecl();
  auto it = writtenParams_.find(canonical);
  if (it == writtenParams_.end()) {
    // Function body not available
    return false;
  }
  // If paramIdx is NOT in the written set the param is read-only
  return it->second.find(paramIdx) == it->second.end();
}

void ParamWriteAnalyzer::analyzeFunction(const FunctionDecl *f) {
  const FunctionDecl *canonical = f->getCanonicalDecl();
  writtenParams_[canonical];

  for (unsigned i = 0; i < f->getNumParams(); i++) {
    const ParmVarDecl *param = f->getParamDecl(i);
    QualType paramType = param->getType();

    // Guard against null types
    if (paramType.isNull())
      continue;

    // Only analyze non-const pointer/reference parameters
    // (Const parameters are already treated correctly by handleCallExpr)
    if (!isFullConstType(paramType) && (isReferenceQualType(paramType) || isPointerQualType(paramType))) {
      if (containsWriteTo(f->getBody(), param, f)) {
        writtenParams_[canonical].insert(i);
      }
    }
  }
}

bool ParamWriteAnalyzer::containsWriteTo(const Stmt *s, const ParmVarDecl *param, const FunctionDecl *parentFunc) const {
  if (!s)
    return false;

  // Check binary assignment operators: param[i] = ..., *param = ...
  if (const auto *bo = dyn_cast<BinaryOperator>(s)) {
    if (bo->isAssignmentOp() || bo->isCompoundAssignmentOp()) {
      if (refersToParam(bo->getLHS(), param))
        return true;
    }
  }

  // Check unary increment/decrement: (*param)++, param[i]--
  if (const auto *uo = dyn_cast<UnaryOperator>(s)) {
    if (uo->isIncrementDecrementOp()) {
      if (refersToParam(uo->getSubExpr(), param))
        return true;
    }
  }

  // An alias or an escaping address may be written outside the direct uses
  // understood here. Keep such parameters mutable until alias tracking exists.
  if (const auto *ds = dyn_cast<DeclStmt>(s)) {
    for (const auto *decl : ds->decls()) {
      if (const auto *var = dyn_cast<VarDecl>(decl)) {
        auto type = var->getType();
        if ((type->isPointerType() || type->isReferenceType() ||
             type->isRecordType()) &&
            refersToParam(var->getInit(), param))
          return true;
      }
    }
  }
  if (const auto *bo = dyn_cast<BinaryOperator>(s)) {
    if (bo->isAssignmentOp() &&
        (bo->getRHS()->getType()->isPointerType() ||
         bo->getRHS()->getType()->isRecordType()) &&
        refersToParam(bo->getRHS(), param))
      return true;
  }
  if (const auto *ret = dyn_cast<ReturnStmt>(s)) {
    if ((parentFunc->getReturnType()->isPointerType() ||
         parentFunc->getReturnType()->isReferenceType()) &&
        refersToParam(ret->getRetValue(), param))
      return true;
  }

  if (const auto *deleted = dyn_cast<CXXDeleteExpr>(s)) {
    if (refersToParam(deleted->getArgument(), param))
      return true;
  }
  if (const auto *op = dyn_cast<CXXOperatorCallExpr>(s)) {
    if (const auto *method =
            dyn_cast_or_null<CXXMethodDecl>(op->getDirectCallee())) {
      if (!method->isConst() && op->getNumArgs() &&
          refersToParam(op->getArg(0), param))
        return true;
    }
  }

  if (const auto *ce = dyn_cast<CallExpr>(s)) {
    const auto *callee = ce->getDirectCallee();
    if (const auto *member = dyn_cast<CXXMemberCallExpr>(ce)) {
      const auto *method = member->getMethodDecl();
      if ((!method || !method->isConst()) &&
          refersToParam(member->getImplicitObjectArgument(), param))
        return true;
    }
    for (unsigned i = 0; i < ce->getNumArgs(); ++i) {
      if (!refersToParam(ce->getArg(i), param))
        continue;
      // Function pointers and variadic arguments have no analyzable contract.
      if (!callee || i >= callee->getNumParams())
        return true;
      auto type = callee->getParamDecl(i)->getType();
      if (!isFullConstType(type) &&
          (isReferenceQualType(type) || isPointerQualType(type)) &&
          !isParamReadOnly(callee, i))
        return true;
    }
  }

  // Recurse into all child
  for (const Stmt *child : s->children()) {
    if (child && containsWriteTo(child, param, parentFunc))
      return true;
  }

  return false;
}

bool ParamWriteAnalyzer::refersToParam(const Expr *e, const ParmVarDecl *param) const {
  if (!e)
    return false;
  e = e->IgnoreParenImpCasts();

  // Direct reference to the parameter variable
  if (const auto *dre = dyn_cast<DeclRefExpr>(e)) {
    return dre->getDecl()->getCanonicalDecl() == param->getCanonicalDecl();
  }

  for (const auto *child : e->children()) {
    if (const auto *expr = dyn_cast_or_null<Expr>(child)) {
      if (refersToParam(expr, param))
        return true;
    }
  }
  return false;
}
