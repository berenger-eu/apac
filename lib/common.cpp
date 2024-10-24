#include "common.hpp"

std::string getCompleteVarDeclStr(const VarDecl &v) {
  std::stringstream SSprint;
  SSprint << v.getType().getAsString() << " " << getVarDeclDefStr(v);
  return SSprint.str();
}
std::string getVarDeclDefStr(const VarDecl &v) {
  std::stringstream SSresult;
  // type varName
  if (v.getInit()) {
    SSresult << v.getNameAsString();
    //= initValue
    switch (v.getInitStyle()) {
    case VarDecl::CallInit:
      SSresult << "(" << getInitString(v) << ")";
      break;
    case VarDecl::CInit:
      SSresult << " = " << getInitString(v);
      break;
    default:
      llvm::errs() << "Unknown Initialization Style\n";
      break;
    }
    SSresult << ";\n";
  }
  return SSresult.str();
}
std::string getVarDeclDeclStr(const VarDecl &v) {
  std::stringstream SSresult;
  // type varName
  SSresult << v.getType().getAsString() << " " << v.getNameAsString() << ";\n";
  return SSresult.str();
}
std::string getStmtAsString(const Stmt *statement, const LangOptions &langOpt,
                            bool noSemi) {
  if (!statement)
    return std::string();
  if (isa<ForStmt>(statement)) {
    const ForStmt *f = cast<ForStmt>(statement);
    std::stringstream ss;
    ss << "for(" << getStmtAsString(f->getInit(), langOpt)
       << getExprAsString(f->getCond(), langOpt) << ";"
       << getExprAsString(f->getInc(), langOpt, true) << ")";
    return ss.str();
  } else if (isa<IfStmt>(statement)) {
    const IfStmt *i = cast<IfStmt>(statement);
    std::stringstream ss;
    ss << "if(" << getExprAsString(i->getCond(), langOpt) << ")";
    return ss.str();
  } else {
    std::string stmtString;
    std::stringstream SSprint;
    PrintingPolicy print_policy(langOpt);
    print_policy.SuppressUnwrittenScope = true;
    llvm::raw_string_ostream stringStreamStmt(stmtString);
    statement->printPretty(stringStreamStmt, NULL, print_policy);
    if (!noSemi && !isa<DeclStmt>(statement))
      stmtString += ";";
    else if (noSemi && isa<DeclStmt>(statement))
      stmtString = stmtString.substr(0, stmtString.size() - 1);
    return stmtString;
  }
}
std::string getStmtAsStringFull(const Stmt *statement,
                                const LangOptions &langOpt) {
  if (!statement)
    return std::string();
  std::string stmtString;
  std::stringstream SSprint;
  PrintingPolicy print_policy(langOpt);
  print_policy.SuppressUnwrittenScope = true;
  llvm::raw_string_ostream stringStreamStmt(stmtString);
  statement->printPretty(stringStreamStmt, NULL, print_policy);
  if (!isa<DeclStmt>(statement))
    stmtString += ";";
  return stmtString;
}
std::string getExprAsString(const Expr *expression, const LangOptions &langOpt,
                            bool noSemi) {
  std::string exprString;
  if (expression != NULL) {

    std::stringstream SSprint;
    PrintingPolicy print_policy(langOpt);
    print_policy.SuppressUnwrittenScope = true;
    llvm::raw_string_ostream stringStreamExpr(exprString);
    expression->printPretty(stringStreamExpr, NULL, print_policy);
  }
  return exprString;
}

std::string getExprAsString(const Expr *expression,
                            const LangOptions &langOpt) {
  std::string exprString;
  if (expression != NULL) {

    std::stringstream SSprint;
    PrintingPolicy print_policy(langOpt);
    print_policy.SuppressUnwrittenScope = true;
    llvm::raw_string_ostream stringStreamExpr(exprString);
    expression->printPretty(stringStreamExpr, NULL, print_policy);
  }
  return exprString;
}

// To verify more clearly if a QualType is a Pointer
bool isPointerQualType(QualType qType) {
  const Type *typeTemp;
  bool returnValue = false;
  // Check for simple pointers type
  if ((typeTemp = qType.getTypePtrOrNull())) {
    returnValue = typeTemp->isPointerType();
  }
  // TODO:Check for more complex pointer types (shared_ptr,unique_ptr,...)
  return returnValue;
}
bool isReferenceQualType(QualType qType) {
  bool returnValue = false;

  returnValue = qType->isReferenceType();
  // We check for special cases where the type is not a simple reference type
  // Here, we check if it's a reference_wrapper
  if (!returnValue) {
    returnValue =
        qType.getAsString().find("reference_wrapper") != std::string::npos;
  }

  return returnValue;
}

QualType getNonReferenceQualType(QualType qType) {
  qType.dump();
  if (qType->isReferenceType()) {
    return qType.getNonReferenceType();
  } else if (qType.getAsString().find("reference_wrapper") !=
             std::string::npos) {
    if (isa<TemplateSpecializationType>(qType)) {
      const TemplateSpecializationType *tst =
          cast<TemplateSpecializationType>(qType);
      return tst->template_arguments().front().getAsType();
    }
  }
  return qType;
}

void getLeafs(Stmt *st, std::vector<Stmt *> &leafs) {
  std::queue<Stmt *> vectNodes;
  vectNodes.push(st);
  int temp = 0;
  while (!vectNodes.empty()) {
    temp++;
    Stmt *s = vectNodes.front();
    auto lastSize = vectNodes.size();
    if (isa<CallExpr>(s)) {
      leafs.push_back(s);
    } else {
      for (auto it = s->child_begin(); it != s->child_end(); ++it) {
        vectNodes.push(*it);
      }
      if (lastSize == vectNodes.size()) {
        leafs.push_back(s);
      }
    }
    vectNodes.pop();
  }
}
std::vector<const DeclRefExpr *> getAllDeclRefExprInsideExpr(const Expr *e) {
  if (!e) {
    return std::vector<const DeclRefExpr *>();
  }
  std::vector<const DeclRefExpr *> vectDeclRefExpr;
  std::queue<const Expr *> vectNodes;
  vectNodes.push(e);
  while (!vectNodes.empty()) {
    const Expr *s = vectNodes.front();
    if (isa<DeclRefExpr>(s) && isa<VarDecl>(cast<DeclRefExpr>(s)->getDecl())) {
      vectDeclRefExpr.push_back(cast<DeclRefExpr>(s));
    } else {
      for (auto it = s->child_begin(); it != s->child_end(); ++it) {
        if (isa<Expr>(*it)) {
          vectNodes.push(cast<Expr>(*it));
        }
      }
    }
    vectNodes.pop();
  }
  return vectDeclRefExpr;
}
const DeclRefExpr *getSingleDeclRefExprInsideExpr(const Expr *e) {
  const DeclRefExpr *returnValue = NULL;
  std::vector<const DeclRefExpr *> vectDeclRefExpr =
      getAllDeclRefExprInsideExpr(e);
  if (vectDeclRefExpr.size() == 1) {
    returnValue = vectDeclRefExpr.front();
  }
  return returnValue;
}

const DeclRefExpr *getArrayBaseDeclRefExpr(const ArraySubscriptExpr *ase) {
  const DeclRefExpr *returnValue = NULL;
  const Expr *base = ase;
  while (isa<ArraySubscriptExpr>(base)) {
    base = cast<ArraySubscriptExpr>(base)->getBase();
    base = base->IgnoreParenImpCasts();
  }
  returnValue = getSingleDeclRefExprInsideExpr(base);
  return returnValue;
}

const ArraySubscriptExpr *getSingleArraySubscriptExprInsideExpr(const Expr *e) {

  std::deque<const ArraySubscriptExpr *> dequeArraySubscriptExpr;
  std::stack<const Expr *> stackExpr;
  const Expr *curExpr = e;

  stackExpr.push(curExpr);
  while (!stackExpr.empty()) {
    curExpr = stackExpr.top();
    stackExpr.pop();
    if (isa<ArraySubscriptExpr>(curExpr)) {
      const ArraySubscriptExpr *ase = cast<ArraySubscriptExpr>(curExpr);
      dequeArraySubscriptExpr.push_front(ase);
    } else {
      for (auto it = curExpr->child_begin(); it != curExpr->child_end(); ++it)
        if (isa<Expr>(*it))
          stackExpr.push(cast<Expr>(*it));
    }
  }
  const ArraySubscriptExpr *returnValue = NULL;
  if (dequeArraySubscriptExpr.size() == 1) {
    returnValue = dequeArraySubscriptExpr.front();
  }
  return returnValue;
}
bool isFullConstType(const QualType &qType) {
  const Type *typeTemp;
  bool workDone = false;
  bool returnValue = true;
  QualType qTypeTemp = qType;
  // We remove the reference if there is one, since it's supposed to be const
  if (isReferenceQualType(qType)) {
    qTypeTemp = qType.getNonReferenceType();
  }
  // We check all types until we reach the last one, we stop if we find a
  // non-const type
  do {
    if (qTypeTemp.isConstQualified()) {
      qTypeTemp = qTypeTemp->getPointeeType();
    } else {
      workDone = true;
      returnValue = false;
    }
  } while ((typeTemp = qTypeTemp.getTypePtrOrNull()) && !workDone);
  return returnValue;
}

int getPtrDepthAccess(QualType qt1, QualType qt2, const ASTContext &aContext) {
  int returnValue = 0;
  // If both types are the same, then we return 0 since they have the same depth
  qt2 = qt2.getSingleStepDesugaredType(aContext);
  if (isReferenceQualType(qt2))
    qt2 = getNonReferenceQualType(qt2);
  if (qt1 != qt2) {
    // If a pointer to type qt1 is equal to qt2, then qt2 is a pointer to type
    // qt1
    if (getPointerToQType(qt1, aContext) == qt2) {
      returnValue = -1;
    }
    // Else, we compare type pointed by qt1 until we find the same type as qt2
    // The number of iteration is the depth of the pointer access
    else {
      while (qt1 != qt2 && qt1.getTypePtrOrNull() && qt2.getTypePtrOrNull()) {
        if (isPointerQualType(qt1)) {
          returnValue++;
          qt1 = qt1->getPointeeType();
        } else if (isReferenceQualType(qt1)) {
          qt1 = getNonReferenceQualType(qt1);
        } else if (qt1->isArrayType() || qt1->isConstantArrayType()) {
          qt1 = aContext.getAsArrayType(qt1)->getElementType();
        }
        qt1 = qt1.getSingleStepDesugaredType(aContext);
      }
    }
  }
  return returnValue;
}

int getPtrDepthAccess(const clang::VarDecl &v, const clang::Expr &e) {
  const QualType &qt1 = v.getType();
  const QualType &qt2 = e.getType();
  return getPtrDepthAccess(qt1, qt2, v.getASTContext());
}

std::deque<const clang::ArraySubscriptExpr *>
getArraySubscripts(const clang::Expr *e) {
  std::deque<const clang::ArraySubscriptExpr *> queueArraySubscriptExpr;
  const Expr *curExpr = e;
  std::stack<const Expr *> stackExpr;
  stackExpr.push(curExpr);
  while (!stackExpr.empty()) {
    curExpr = stackExpr.top();
    stackExpr.pop();
    if (isa<ArraySubscriptExpr>(curExpr)) {
      const ArraySubscriptExpr *ase = cast<ArraySubscriptExpr>(curExpr);
      queueArraySubscriptExpr.push_front(ase);
      stackExpr.push(ase->getBase());
    } else {
      for (auto it = curExpr->child_begin(); it != curExpr->child_end(); ++it)
        if (isa<Expr>(*it))
          stackExpr.push(cast<Expr>(*it));
    }
  }
  return queueArraySubscriptExpr;
}
std::vector<const clang::Expr *>
getArraySubscriptsIndexes(const clang::Expr *e) {
  std::vector<const clang::Expr *> vectArraySubscriptExprIndexes;
  std::deque<const clang::ArraySubscriptExpr *> queueArraySubscriptExpr =
      getArraySubscripts(e);
  for (auto &expr : queueArraySubscriptExpr) {
    vectArraySubscriptExprIndexes.push_back(expr->getIdx());
  }
  return vectArraySubscriptExprIndexes;
}
std::vector<int> getArraySubscriptsIndexesValues(const clang::Expr *e) {
  std::vector<int> vectArraySubscriptExpr;
  std::vector<const Expr *> vectExpr = getArraySubscriptsIndexes(e);
  if (isa<ArraySubscriptExpr>(e)) {
    auto arraySubscriptExpr = cast<ArraySubscriptExpr>(e);
    if (getSingleDeclRefExprInsideExpr(arraySubscriptExpr->getBase())) {
      auto base = cast<VarDecl>(
          getSingleDeclRefExprInsideExpr(arraySubscriptExpr->getBase())
              ->getDecl());
      for (auto &expr : vectExpr) {
        // TODO: Handle cases with evaluable expressions (5+4 , ...)
        Expr::EvalResult result;
        if (expr->EvaluateAsInt(result, base->getASTContext()))
          vectArraySubscriptExpr.push_back(result.Val.getInt().getSExtValue());
        else
          vectArraySubscriptExpr.push_back(-1);
      }
    }
  }
  return vectArraySubscriptExpr;
}