#include "ASTTaskGraphVisitor.hpp"

void ASTTaskGraphVisitor::computeAliasesForRHS(
    const Expr *expression,
    std::unordered_set<std::shared_ptr<aliasArg>> &aliases,
    Instruction &instr) {

  int depth;
  llvm::errs() << "compute\n";

  const Expr *rhs = expression->IgnoreParenImpCasts();
  const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(rhs);
  llvm::errs() << (d == nullptr) << "\n";
  const auto a = getSingleArraySubscriptExprInsideExpr(rhs);
  // We look for variables aliased in a given expression
  // If we access a variable, then we found the aliases of the variable really
  // accessed
  //(*p), the variable is p, but in reality we access *p, so we need to look for
  // the aliases of *p
  llvm::errs() << "compute\n";
  if (a) {
    ;
  } else if (d) {
    // Aliases will be values that refer (or might refer) to the same memory
    // location
    // TODO:Modif ici pour ajout alias aux nodes
    llvm::errs() << "DeclRefExpr\n";
    const VarDecl *v = cast<VarDecl>(d->getDecl());
    AliasType type;
    if (isPointerQualType(v->getType()))
      type = Pointer;
    else if (isReferenceQualType(v->getType()))
      type = Reference;
    else
      type = Variable;
    const auto mainAlias = aliasTable.getOrAddAliasArg(v, type);
    aliases.insert(mainAlias);
    depth = getPtrDepthAccess(*v, *rhs);
    // We retrive the pointed values or the references
    aliasTable.getModifiedVariables(aliases, depth);
    for (auto &alias : aliases)
      if (mainAlias != alias)
        instr.curAliases.insert({mainAlias, alias});
  }
  // Handle CallExpr ( int * p=min(&a,&b) , p might point to a or b or something
  // new)
  else if (isa<CallExpr>(rhs)) {
    const CallExpr *c = cast<CallExpr>(rhs);
    const FunctionDecl *f = c->getDirectCallee();
    for (unsigned int i = 0; i < c->getNumArgs(); i++) {
      const ParmVarDecl *curParam = f->getParamDecl(i);
      if ((isPointerQualType(curParam->getType()) ||
           isReferenceQualType(curParam->getType())) &&
          !isFullConstType(curParam->getType())) {
        const Expr *arg = c->getArg(i);
        std::unordered_set<std::shared_ptr<aliasArg>> subAliases;
        computeAliasesForRHS(arg, subAliases, instr);
        for (auto &alias : subAliases)
          aliases.insert(alias);
        if (isa<VarDecl>(getSingleDeclRefExprInsideExpr(arg)->getDecl())) {
          const VarDecl *v =
              cast<VarDecl>(getSingleDeclRefExprInsideExpr(arg)->getDecl());
          if (getPtrDepthAccess(*v, *arg) == -1) {
            AliasType type;
            if (isPointerQualType(v->getType()))
              type = Pointer;
            else if (isReferenceQualType(v->getType()))
              type = Reference;
            else
              type = Variable;
            auto alias = aliasTable.getOrAddAliasArg(v, type);
            aliases.insert(alias);
          }
        }
      }
    }
    // TODO: Handle CallExpr
  } else {
    llvm::errs() << "Unhandled expression\n";
    handleStmt(*rhs, instr);
  }
  llvm::errs() << "done with compute\n";
}

void ASTTaskGraphVisitor::handleCXXOperatorCallExpr(
    const CXXOperatorCallExpr &c, Instruction &instr, bool isWrite) {

  if (c.isAssignmentOp() && c.getNumArgs() == 2) {

    if (isa<DeclRefExpr>(c.getArg(0)) &&
        isReferenceQualType(c.getArg(0)->getType())) {
      const VarDecl *v =
          cast<VarDecl>(cast<DeclRefExpr>(c.getArg(0))->getDecl());
      const VarDecl *v2;
      std::vector<int> indexes;
      auto aliasRef = aliasTable.getOrAddAliasArg(v, Reference);
      const DeclRefExpr *d;
      const ArraySubscriptExpr *array;
      if ((array = getSingleArraySubscriptExprInsideExpr(c.getArg(1))) !=
          nullptr) {
        const auto baseDeclExpr =
            getSingleDeclRefExprInsideExpr(array->getBase());
        v2 = cast<VarDecl>(baseDeclExpr->getDecl());
        indexes = getArraySubscriptsIndexesValues(array);

      } else if ((d = getSingleDeclRefExprInsideExpr(c.getArg(1))) != nullptr) {
        v2 = cast<VarDecl>(d->getDecl());
      } else {
        handleStmt(*c.getArg(1), instr);
        return;
      }

      AliasType type;
      if (isPointerQualType(c.getArg(1)->getType()))
        type = Pointer;
      else if (isReferenceQualType(c.getArg(1)->getType()))
        type = Reference;
      else
        type = Variable;
      auto aliasVar = aliasTable.getOrAddAliasArg(v2, type, indexes);
      aliasTable.addAliasReference(aliasVar, aliasRef);
      addDependencyRead(instr, aliasVar);
      addDependencyWrite(instr, aliasRef);
    }
  } else
    for (const auto &arg : c.arguments())
      handleStmt(*arg, instr, isWrite);
}

void ASTTaskGraphVisitor::handleUnaryOperator(const UnaryOperator &uop,
                                              Instruction &curInstr,
                                              bool isWrite) {
  Expr *subExpr = uop.getSubExpr();
  // If we have a variable
  const VarDecl *mainVariable = nullptr;
  std::vector<int> indexes;
  const DeclRefExpr *d;
  const ArraySubscriptExpr *array;
  if ((array = getSingleArraySubscriptExprInsideExpr(subExpr)) != nullptr) {
    const auto baseDeclExpr = getSingleDeclRefExprInsideExpr(array->getBase());
    mainVariable = cast<VarDecl>(baseDeclExpr->getDecl());
    indexes = getArraySubscriptsIndexesValues(array);
  } else if ((d = getSingleDeclRefExprInsideExpr(subExpr)) != nullptr) {
    // and we increment or decrement it, then it's a read and a write
    mainVariable = cast<VarDecl>(d->getDecl());
  } else {
    handleStmt(*subExpr, curInstr);
    return;
  }
  const Expr *arrayOrDeclExpr;
  if (array)
    arrayOrDeclExpr = array;
  else
    arrayOrDeclExpr = d;
  std::unordered_set<std::shared_ptr<aliasArg>> aliases;
  computeAliasesForRHS(&uop, aliases, curInstr);
  for (auto alias : aliases)
    addDependencyRead(curInstr, alias);

  // TODO: check if other cases are read and/or write
  if (uop.isIncrementOp() || uop.isDecrementOp())
    for (auto &alias : aliases)
      addDependencyWrite(curInstr, alias);

  if ((getPtrDepthAccess(arrayOrDeclExpr->getType(), subExpr->getType(),
                         mainVariable->getASTContext())) > 0)
    addDependencyRead(
        curInstr, aliasTable.getOrAddAliasArg(mainVariable, Pointer, indexes));

  // Otherwise, unary expression affects a temporary value so we ignore it but
  // still look through the expression
  else
    handleStmt(*subExpr, curInstr);
}

void ASTTaskGraphVisitor::handleBinaryOperator(const BinaryOperator &bop,
                                               Instruction &curInstr,
                                               bool isWrite) {
  llvm::errs() << "Binary operator\n";
  // Special case for assignment operators, because it is a write
  if (bop.isAssignmentOp()) {
    // Most likely unnecessary since left side has to be a lvalue because of the
    // assignment operator
    const VarDecl *mainVariable = nullptr;
    const DeclRefExpr *d;
    const ArraySubscriptExpr *array;
    std::vector<int> indexes;
    if ((array = getSingleArraySubscriptExprInsideExpr(bop.getLHS())) !=
        nullptr) {
      const auto baseDeclExpr =
          getSingleDeclRefExprInsideExpr(array->getBase());
      mainVariable = cast<VarDecl>(baseDeclExpr->getDecl());
      indexes = getArraySubscriptsIndexesValues(array);
    } else if ((d = getSingleDeclRefExprInsideExpr(bop.getLHS())))
      mainVariable = cast<VarDecl>(d->getDecl());
    else
      handleStmt(*bop.getLHS(), curInstr, true);
    const Expr *declOrArrayExpr;
    if (array)
      declOrArrayExpr = array;
    else
      declOrArrayExpr = d;
    std::unordered_set<std::shared_ptr<aliasArg>> aliasesLeft;
    // setLeftVars.insert(v);
    llvm::errs() << "LHS: ";
    computeAliasesForRHS(bop.getLHS(), aliasesLeft, curInstr);
    llvm::errs() << "Size of aliases: " << aliasesLeft.size() << "\n";
    // aliasTable.getModifiedVariables(setLeftVars,depth);

    if (isPointerQualType(bop.getLHS()->getType())) {
      // Separate this part in a different function to make it more
      // understandable

      // If a single pointer is aliased, then we know that its aliased
      // elements will change If there are multiple, it's because we can't be
      // sure which one is aliased, so we can't remove the dependencies
      llvm::errs() << "Size of aliasesd: " << aliasesLeft.size() << "\n";
      if (aliasesLeft.size() == 1)
        aliasTable.removeDependencyPtr(*aliasesLeft.begin());

      std::unordered_set<std::shared_ptr<aliasArg>> aliasesRHS;
      computeAliasesForRHS(bop.getRHS(), aliasesRHS, curInstr);
      llvm::errs() << "Size of aliaseszez: " << aliasesRHS.size()
                   << aliasesLeft.size() << "\n";
      for (auto &alias : aliasesRHS)
        alias->dump();

      if (getPtrDepthAccess(declOrArrayExpr->getType(), bop.getRHS()->getType(),
                            mainVariable->getASTContext()) == -1) {
        for (auto &aliasLeft : aliasesLeft) {
          auto aliasRight = aliasTable.getOrAddAliasArg(
              mainVariable, AliasType::Pointer, indexes);
          aliasTable.addAliasPtr(aliasRight, aliasLeft);
        }
      }
      for (auto &aliasLeft : aliasesLeft)
        for (auto &aliasRight : aliasesRHS)
          aliasTable.addAliasPtr(aliasRight, aliasLeft);
    }
    // It's an assignment, so all aliases are written
    for (auto &alias : aliasesLeft) {
      addDependencyWrite(curInstr, alias);
      if (isa<CompoundAssignOperator>(bop))
        addDependencyRead(curInstr, alias);
    }
    int depth =
        getPtrDepthAccess(declOrArrayExpr->getType(), bop.getLHS()->getType(),
                          mainVariable->getASTContext());
    // If we have a pointer, then the aliases are related to the pointed
    // values when depth > 0 So we have to add a read dependency on the
    // pointer itself
    AliasType type;
    if (isPointerQualType(bop.getLHS()->getType()))
      type = Pointer;
    else if (isReferenceQualType(bop.getLHS()->getType()))
      type = Reference;
    else
      type = Variable;
    auto aliasMain = aliasTable.getOrAddAliasArg(mainVariable, type, indexes);
    if (depth > 0)
      addDependencyRead(curInstr, aliasMain);
    handleStmt(*bop.getRHS(), curInstr);
  }
  // Otherwise, we just look through the expression on both sides
  else {
    handleStmt(*bop.getLHS(), curInstr);
    handleStmt(*bop.getRHS(), curInstr);
  }
  llvm::errs() << "Done with binary operator\n";
}
void ASTTaskGraphVisitor::handleMemberCallExpr(const CXXMemberCallExpr &c,
                                               Instruction &curInstr,
                                               bool isWrite) {
  Expr *obj = c.getImplicitObjectArgument();
  AliasType type;
  if (isPointerQualType(obj->getType()))
    type = Pointer;
  else if (isReferenceQualType(obj->getType()))
    type = Reference;
  else
    type = Variable;
  if (isa<DeclRefExpr>(obj)) {
    const auto alias = aliasTable.getOrAddAliasArg(
        cast<VarDecl>(cast<DeclRefExpr>(obj)->getDecl()), type);
    addDependencyRead(curInstr, alias);
    if (!c.getMethodDecl()->isConst())
      addDependencyWrite(curInstr, alias);
  } else
    handleStmt(*obj, curInstr);
  handleCallExpr(c, curInstr);
}
void ASTTaskGraphVisitor::handleCallExpr(const CallExpr &c,
                                         Instruction &curInstr, bool isWrite) {
  // TODO: Methods and read/write on object/data ?
  const FunctionDecl &f = *(c.getDirectCallee());
  // We look though each parameter of the function
  for (unsigned int i = 0; i < f.getNumParams(); i++) {
    const ParmVarDecl &p = *(f.getParamDecl(i));
    const Expr *b = c.getArg(i);
    // If we have a variable, then there might be a write
    if (isInExceptionList(p)) {
      const Expr *d = b;
      if (isa<CXXBindTemporaryExpr>(d))
        d = cast<CXXBindTemporaryExpr>(*(b->IgnoreCasts())).getSubExpr();
      if (isa<CXXConstructExpr>(d))
        d = cast<CXXConstructExpr>(d)->getArg(0);
      if (isa<DeclRefExpr>(d->IgnoreParenImpCasts())) {
        const VarDecl *v = cast<VarDecl>(
            cast<DeclRefExpr>(d->IgnoreParenImpCasts())->getDecl());
        AliasType type;
        if (isPointerQualType(p.getType()))
          type = Pointer;
        else if (isReferenceQualType(p.getType()))
          type = Reference;
        else
          type = Variable;
        const auto alias = aliasTable.getOrAddAliasArg(v, type);
        addDependencyRead(curInstr, alias);
        addDependencyWrite(curInstr, alias);
      }
    } else if (!(isFullConstType(p.getType())) &&
               (isReferenceQualType(p.getType()) ||
                isPointerQualType(p.getType()))) {
      const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(b);
      // If the parameter can be modified (parameter is either a reference or a
      // pointer AND it's not completely const)
      //   then there might be a write, so we assume there is one
      if (d) {
        const VarDecl *v = cast<VarDecl>(d->getDecl());
        AliasType type;
        if (isPointerQualType(p.getType()))
          type = Pointer;
        else if (isReferenceQualType(p.getType()))
          type = Reference;
        else
          type = Variable;
        auto alias = aliasTable.getOrAddAliasArg(v, type);

        addDependencyRead(curInstr, alias);
        addDependencyWrite(curInstr, alias);
        std::unordered_set<std::shared_ptr<aliasArg>> aliases =
            aliasTable.getAliased(alias);

        for (auto &ali : aliases) {
          addDependencyRead(curInstr, ali);
          addDependencyWrite(curInstr, ali);
        }
      } else
        llvm::errs() << "Failed to find DeclRefExpr\n";
    }
    // Otherwise, we look through the expression since it is the same as looking
    // through any expression
    else
      handleStmt(*b, curInstr);
  }
}
void ASTTaskGraphVisitor::handleStmt(const Stmt &st, Instruction &instr,
                                     bool isWrite) {
  // Simple switch case to call the respective handle method, except for
  // DeclRefExpr which is a variable so it is a read
  if (isa<ReturnStmt>(st)) {
    if (cast<ReturnStmt>(st).getRetValue())
      handleStmt(*(cast<ReturnStmt>(st).getRetValue()), instr, isWrite);
  } else if (isa<Expr>(st)) {
    const Expr &curExp = *(cast<Expr>(st).IgnoreParenImpCasts());
    if (isa<CXXOperatorCallExpr>(curExp))
      handleCXXOperatorCallExpr(cast<CXXOperatorCallExpr>(curExp), instr,
                                isWrite);
    else if (isa<UnaryOperator>(curExp))
      handleUnaryOperator(cast<UnaryOperator>(curExp), instr, isWrite);
    else if (isa<BinaryOperator>(curExp))
      handleBinaryOperator(cast<BinaryOperator>(curExp), instr, isWrite);
    else if (isa<CXXMemberCallExpr>(curExp))
      handleMemberCallExpr(cast<CXXMemberCallExpr>(curExp), instr, isWrite);
    else if (isa<CallExpr>(curExp))
      handleCallExpr(cast<CallExpr>(curExp), instr, isWrite);
    else if (isa<DeclRefExpr>(curExp)) {
      const VarDecl *v = cast<VarDecl>(cast<DeclRefExpr>(curExp).getDecl());
      AliasType type;
      if (isPointerQualType(v->getType()))
        type = Pointer;
      else if (isReferenceQualType(v->getType()))
        type = Reference;
      else
        type = Variable;
      auto alias = aliasTable.getOrAddAliasArg(v, type);
      addDependencyRead(instr, alias);
      if (isWrite)
        addDependencyWrite(instr, alias);
    } else if (isa<ArraySubscriptExpr>(curExp)) {
      ;
    }
    // Ignored expressions case
    else if (isa<IntegerLiteral>(curExp)) {
      ; // Do nothing
    } else {
      llvm::errs() << "Unhandled expression\n";
      llvm::errs() << TheRewriter.getSourceMgr()
                          .getPresumedLoc(curExp.getBeginLoc())
                          .getFilename()
                   << ":";
      curExp.dump();
    }
  }
}

bool ASTTaskGraphVisitor::TraverseFunctionDecl(FunctionDecl *f) {
  if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc()))
    return true;

  // If function is not in headers and has a body and is a definition, then we
  // traverse it recursively Using traverse we can avoid visiting nodes that we
  // don't need
  if (f->getBody() && f->isThisDeclarationADefinition()) {
    functionsInstructionsVector.push_back(std::vector<Instruction>());
    return RecursiveASTVisitor::TraverseFunctionDecl(f);
  }
  return true;
}

bool ASTTaskGraphVisitor::TraverseForStmt(ForStmt *f) {
  if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc()))
    return true;
  bool res = true;

  bool oldValIgnore = ignoreStmtPragma;
  StmtOrder *outerInstrOrder = currentOrderManager;
  if (!ignoreStmtPragma)
    currentOrderManager->addInstructionToManager(f);
  if (currentOrderManager->getSubStmtOrder(f) != nullptr)
    currentOrderManager = (outerInstrOrder->getSubStmtOrder(f)).get();
  llvm::errs() << ((outerInstrOrder->getSubStmtOrder(f)).get() == nullptr)
               << "End ForStmt\n";
  Instruction compInstr(f, getStmtAsString(f, TheRewriter.getLangOpts()), true,
                        0);
  functionsInstructionsVector.push_back(std::vector<Instruction>());
  ignoreStmtPragma = true;
  if (!(RecursiveASTVisitor::TraverseStmt(f->getCond()) &&
        RecursiveASTVisitor::TraverseStmt(f->getInc()) &&
        RecursiveASTVisitor::TraverseStmt(f->getInit()))) {
    ignoreStmtPragma = oldValIgnore;
    return false;
  }
  ignoreStmtPragma = false;
  if (!RecursiveASTVisitor::TraverseStmt(f->getBody())) {
    ignoreStmtPragma = oldValIgnore;
    return false;
  }
  ignoreStmtPragma = oldValIgnore;
  currentOrderManager = outerInstrOrder;

  compInstr.scopedInstructions = functionsInstructionsVector.back();
  for (auto &instr : compInstr.scopedInstructions) {
    for (auto &dep : instr.dependencies) {
      compInstr.dependencies.insert(dep);
    }
    if (instr.complexInstruction) {
      compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
    }
    compInstr.scopedInstructionsNumber++;
  }
  functionsInstructionsVector.pop_back();
  functionsInstructionsVector.back().push_back(compInstr);

  return res;
}
// TODO: refacto
bool ASTTaskGraphVisitor::TraverseIfStmt(IfStmt *i) {
  if (isInHeaders(TheRewriter.getSourceMgr(), i->getBeginLoc()))
    return true;
  bool res = true;

  Instruction compInstr(i, getStmtAsString(i, TheRewriter.getLangOpts()), true,
                        0);
  functionsInstructionsVector.push_back(std::vector<Instruction>());
  // res=RecursiveASTVisitor::TraverseIfStmt(i);
  if (i->getThen() && isa<CompoundStmt>(i->getThen())) {
    CompoundStmt *c = cast<CompoundStmt>(i->getThen());
    Instruction compInstr(c, "if", true, 0);
    functionsInstructionsVector.push_back(std::vector<Instruction>());
    res = RecursiveASTVisitor::TraverseCompoundStmt(c);
    compInstr.scopedInstructions = functionsInstructionsVector.back();
    for (auto &instr : compInstr.scopedInstructions) {
      for (auto &dep : instr.dependencies) {
        compInstr.dependencies.insert(dep);
      }
      if (instr.complexInstruction) {
        compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
      }
      compInstr.scopedInstructionsNumber++;
    }
    functionsInstructionsVector.pop_back();
    functionsInstructionsVector.back().push_back(compInstr);
  }
  if (i->getElse()) {
    if (isa<CompoundStmt>(i->getElse())) {

      CompoundStmt *c = cast<CompoundStmt>(i->getElse());
      Instruction compInstr(c, "else", true, 0);

      functionsInstructionsVector.push_back(std::vector<Instruction>());
      res = RecursiveASTVisitor::TraverseCompoundStmt(c);
      compInstr.scopedInstructions = functionsInstructionsVector.back();
      for (auto &instr : compInstr.scopedInstructions) {
        for (auto &dep : instr.dependencies) {
          compInstr.dependencies.insert(dep);
        }
        if (instr.complexInstruction) {
          compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
        }
        compInstr.scopedInstructionsNumber++;
      }
      functionsInstructionsVector.pop_back();
      functionsInstructionsVector.back().push_back(compInstr);
    } else {
      Instruction instr(i->getElse(), "else", true, 0);
      functionsInstructionsVector.push_back(std::vector<Instruction>());
      res = RecursiveASTVisitor::TraverseStmt(i->getElse());
      compInstr.scopedInstructions = functionsInstructionsVector.back();
      for (auto &instr : compInstr.scopedInstructions) {
        for (auto &dep : instr.dependencies) {
          compInstr.dependencies.insert(dep);
        }
        if (instr.complexInstruction) {
          compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
        }
        compInstr.scopedInstructionsNumber++;
      }
      functionsInstructionsVector.pop_back();
      functionsInstructionsVector.back().push_back(compInstr);
    }
  }
  compInstr.scopedInstructions = functionsInstructionsVector.back();
  for (auto &instr : compInstr.scopedInstructions) {
    for (auto &dep : instr.dependencies) {
      compInstr.dependencies.insert(dep);
    }
    if (instr.complexInstruction) {
      compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
    }
    compInstr.scopedInstructionsNumber++;
  }
  functionsInstructionsVector.pop_back();
  functionsInstructionsVector.back().push_back(compInstr);

  return res;
}