#include "ParamWriteAnalyzer.hpp"

void ParamWriteAnalyzer::analyzeTranslationUnit(ASTContext &Ctx) {
  ctx_ = &Ctx;
  TraverseDecl(Ctx.getTranslationUnitDecl());
}

bool ParamWriteAnalyzer::VisitFunctionDecl(FunctionDecl *f) {
  if (f->hasBody() && f->isThisDeclarationADefinition()) {
    // Skip functions defined in headers (standard library, etc.)
    if (ctx_) {
      SourceManager &sm = ctx_->getSourceManager();
      if (isInHeaders(sm, f->getLocation()))
        return true;
    }
    analyzeFunction(f);
  }
  return true;
}

bool ParamWriteAnalyzer::isParamReadOnly(const FunctionDecl *f, unsigned paramIdx) const {
  if (!f)
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

  // Check function calls: if param is passed to another function 
  // as a non-const pointer/reference we conservatively assume write
  // Skip self-recursive calls : the recursive call doesn't introduce new writes
  if (const auto *ce = dyn_cast<CallExpr>(s)) {
    if (const FunctionDecl *callee = ce->getDirectCallee()) {
      if (callee->getCanonicalDecl() != parentFunc->getCanonicalDecl()) {
        // Skip calls to APAC internal helpers (_apac_header.hpp)
        // and already analyzed read-only calls
        bool skipCallee = false;
        if (ctx_) {
          SourceManager &sm = ctx_->getSourceManager();
          if (sm.getFilename(callee->getLocation()).contains("_apac_header.hpp"))
            skipCallee = true;
        }
        if (!skipCallee) {
          for (unsigned i = 0; i < callee->getNumParams() && i < ce->getNumArgs(); i++) {
            const ParmVarDecl *calleeParam = callee->getParamDecl(i);
            QualType calleeParamType = calleeParam->getType();
            if (calleeParamType.isNull())
              continue;
            if (!isFullConstType(calleeParamType) &&
                (isReferenceQualType(calleeParamType) ||
                 isPointerQualType(calleeParamType))) {
              if (isParamReadOnly(callee, i))
                continue;
              if (refersToParam(ce->getArg(i), param))
                return true;
            }
          }
        }
      }
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

  // Dereference of the parameter: *param, **param, etc.
  if (const auto *uo = dyn_cast<UnaryOperator>(e)) {
    if (uo->getOpcode() == UO_Deref) {
      return refersToParam(uo->getSubExpr(), param);
    }
  }

  // Array subs: param[i], param[i][j], etc.
  if (const auto *ase = dyn_cast<ArraySubscriptExpr>(e)) {
    return refersToParam(ase->getBase(), param);
  }

  // Member access: param->member, (*param).member
  if (const auto *me = dyn_cast<MemberExpr>(e)) {
    return refersToParam(me->getBase(), param);
  }

  return false;
}
