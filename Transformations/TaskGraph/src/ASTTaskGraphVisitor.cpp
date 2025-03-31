#include "ASTTaskGraphVisitor.hpp"

void ASTTaskGraphVisitor::addDependenciesRead(Instruction &instr,
                                              const Expr *e) {
  // Two cases : DeclRefExpr (variables) and ArraySubscriptExpr (arrays)
  // Main goal of the function is to handle new read dependencies related to use
  // of arrays (e.g. tab[5] = tab[a]+x), here "a" can be detected as a read
  // dependency with this function)
  auto allDeclRefVar = getAllReadDeclRefExprInsideExpr(e);
  for (auto d : allDeclRefVar) {
    auto alias = aliasTable.getOrAddAliasArg(d);
    addDependencyRead(instr, alias);
  }
  // TODO : Handle ArraySubscriptExpr (only for cases such as tab[ tab [5] ],
  // where tab[5] would not be detected otherwise) But tab[-1] (the outer access
  // to tab) would be detected
}
std::vector<const DeclRefExpr *>
getAllReadDeclRefExprInsideExpr(const Expr *e) {
  if (!e) {
    return std::vector<const DeclRefExpr *>();
  }
  std::vector<const DeclRefExpr *> vectDeclRefExpr;
  std::queue<const Expr *> vectNodes;
  bool stopIter = false;
  vectNodes.push(e);
  while (!vectNodes.empty()) {
    const Expr *s = vectNodes.front();
    // If we have a unary operator, we ignore it if it is a dereference of a
    // single variable (so &a, but not &tab[a])
    if (isa<UnaryOperator>(s)) {
      const auto u = cast<UnaryOperator>(s);
      if (u->getOpcode() == UO_AddrOf &&
          isa<DeclRefExpr>(u->getSubExpr()->IgnoreImpCasts())) {
        stopIter = true;
      }
    }
    if (stopIter) {
      stopIter = false;
    } else if (isa<DeclRefExpr>(s) &&
               isa<VarDecl>(cast<DeclRefExpr>(s)->getDecl())) {
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
void ASTTaskGraphVisitor::computeAliasesForRHS(
    const Expr *expression,
    std::unordered_set<std::shared_ptr<aliasArg>> &aliases,
    Instruction &instr) {

  int depth;

  const Expr *rhs = expression->IgnoreParenImpCasts();
  const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(rhs);
  const auto a = getSingleArraySubscriptExprInsideExpr(rhs);
  // We look for variables aliased in a given expression
  // If we access a variable, then we find the aliases of the variable really
  // accessed
  //(*p), the variable is p, but in reality we access *p, so we need to look
  // for
  // the aliases of *p
  if (a) {
    // const VarDecl *base =
    // cast<VarDecl>(getArrayBaseDeclRefExpr(a)->getDecl());
    const auto indexes = getArraySubscriptsIndexesValues(a);
    llvm::errs() << "test\n";
    const auto mainAlias = aliasTable.getOrAddAliasArg(a);
    aliases.insert(mainAlias);
    mainAlias->dump();
    llvm::errs() << "123: " << mainAlias->varAsString() << "\n";
    llvm::errs() << "Indexes: ";
    for (auto &index : indexes)
      llvm::errs() << index << " ";
    llvm::errs() << "\n";
  } else if (d) {
    // Aliases will be values that refer (or might refer) to the same memory
    // location
    // TODO:Modif ici pour ajout alias aux nodes
    const VarDecl *v = cast<VarDecl>(d->getDecl());
    const auto mainAlias = aliasTable.getOrAddAliasArg(d);
    aliases.insert(mainAlias);
    depth = getPtrDepthAccess(*v, *rhs);
    // We retrive the pointed values or the references
    if (depth != -1)
      aliasTable.getModifiedVariables(aliases, depth);
    for (auto &alias : aliases)
      if (mainAlias != alias)
        instr.curAliases.insert({mainAlias, alias});
  }
  // Handle CallExpr ( int * p=min(&a,&b) , p might point to a or b or
  // something new)
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
            auto alias = aliasTable.getOrAddAliasArg(
                getSingleDeclRefExprInsideExpr(arg));
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
}

void ASTTaskGraphVisitor::handleCXXOperatorCallExpr(
    const CXXOperatorCallExpr &c, Instruction &instr, bool isWrite) {
  // Affectation
  if (c.isAssignmentOp() && c.getNumArgs() == 2) {
    if (isa<DeclRefExpr>(c.getArg(0)) &&
        isReferenceQualType(c.getArg(0)->getType())) {
      // const VarDecl *v =
      //     cast<VarDecl>(cast<DeclRefExpr>(c.getArg(0))->getDecl());
      // const VarDecl *v2;
      std::vector<int> indexes;
      auto aliasRef = aliasTable.getOrAddAliasArg(c.getArg(0), Reference);
      const DeclRefExpr *d;
      const ArraySubscriptExpr *array;
      std::vector<const Expr *> declOrArray;
      if ((array = getSingleArraySubscriptExprInsideExpr(c.getArg(1))) !=
          nullptr) {
        // const auto baseDeclExpr =
        //     getSingleDeclRefExprInsideExpr(getArrayBaseDeclRefExpr(array));
        //  v2 = cast<VarDecl>(baseDeclExpr->getDecl());
        indexes = getArraySubscriptsIndexesValues(array);
        declOrArray.push_back(array);

      } else if ((d = getSingleDeclRefExprInsideExpr(c.getArg(1))) != nullptr) {
        // v2 = cast<VarDecl>(d->getDecl());
        declOrArray.push_back(d);

      } else {
        std::vector<const CallExpr *> callExprVect;
        getLeafsOfType<CallExpr>(c.getArg(1), callExprVect);
        if (!callExprVect.empty()) {

          std::vector<QualType> types;
          auto rightCall = callExprVect.front();
          auto functionCalled = rightCall->getDirectCallee();
          auto calQType = rightCall->getType();
          std::vector<const Expr *> arguments;
          for (auto arg : rightCall->arguments())
            arguments.push_back(arg);
          getLowestType(
              arguments, types,
              callExprVect.front()->getDirectCallee()->getASTContext());
          for (int i = 0; i < arguments.size(); i++) {
            // TODO: Handle Pointers
            if ((calQType == types[i]) &&
                (isReferenceQualType(
                    functionCalled->getParamDecl(i)->getType()))) {

              declOrArray.push_back(arguments[i]);
            }
          }
        } else {
          handleStmt(*c.getArg(1), instr);
          return;
        }
      }
      bool refAliasedEmpty = aliasRef->aliased.empty();
      for (auto declOrArray : declOrArray) {
        declOrArray->dump();
      }
      // AliasType type = getAliasType(declOrArray);
      for (auto declOrArrayExpr : declOrArray) {
        auto aliasVar = aliasTable.getOrAddAliasArg(declOrArrayExpr);
        if (aliasRef->type == Reference && refAliasedEmpty)
          aliasTable.addAliasReference(aliasVar, aliasRef);
        // aliasTable.addAliasReference(aliasVar, aliasRef);
        addDependencyRead(instr, aliasVar);
      }
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
  // Pointer either to the array or to the decl
  const Expr *arrayOrDeclExpr = initVarFromExpr(subExpr, mainVariable, indexes);
  // If it's an operation on an array, then we look for the base variable, and
  // the indexes
  if (arrayOrDeclExpr == nullptr) {
    handleStmt(*subExpr, curInstr);
    return;
  }
  // If the operation is on the address of the expression, then we handle it
  // generically
  if (getPtrDepthAccess(arrayOrDeclExpr->getType(), uop.getType(),
                        mainVariable->getASTContext()) == -1) {
    handleStmt(*subExpr, curInstr, false, false);
    return;
  }
  std::unordered_set<std::shared_ptr<aliasArg>> aliases;
  computeAliasesForRHS(&uop, aliases, curInstr);
  for (auto alias : aliases)
    addDependencyRead(curInstr, alias);

  int depth = (getPtrDepthAccess(arrayOrDeclExpr->getType(), subExpr->getType(),
                                 mainVariable->getASTContext()));
  // TODO: check if other cases are read and/or write
  // Add dependencies for write
  if (uop.isIncrementOp() || uop.isDecrementOp())
    for (auto &alias : aliases)
      addDependencyWrite(curInstr, alias);

  if (depth > 0) {
    auto aliasDecl = aliasTable.getOrAddAliasArg(arrayOrDeclExpr);
    std::unordered_set<std::shared_ptr<aliasArg>> aliases;
    aliases.insert(aliasDecl);
    aliasTable.getPointerAccessedVariables(aliases, depth);
    for (auto alias : aliases)
      addDependencyRead(curInstr, alias);
  }

  // Otherwise, unary expression affects a temporary value so we ignore it but
  // still look through the expression
  else
    handleStmt(*subExpr, curInstr);
}

void ASTTaskGraphVisitor::handleBinaryOperator(const BinaryOperator &bop,
                                               Instruction &curInstr,
                                               bool isWrite) {
  // Special case for assignment operators, because it is a write
  if (bop.isAssignmentOp()) {
    handleBinaryAssignment(bop, curInstr, isWrite);
  }
  // Otherwise, we just look through the expression on both sides
  else {
    handleStmt(*bop.getLHS(), curInstr);
    handleStmt(*bop.getRHS(), curInstr);
  }
}
void ASTTaskGraphVisitor::handleBinaryAssignment(const BinaryOperator &bop,
                                                 Instruction &curInstr,
                                                 bool isWrite) {
  // Most likely unnecessary since left side has to be a lvalue because of
  // the assignment operator
  const VarDecl *mainVariable = nullptr;
  std::vector<int> indexes;
  const Expr *declOrArrayExpr =
      initVarFromExpr(bop.getLHS(), mainVariable, indexes);
  if (declOrArrayExpr == nullptr) {
    handleStmt(*bop.getLHS(), curInstr, true);
    return;
  }

  // TODO:Error for member expr

  std::unordered_set<std::shared_ptr<aliasArg>> aliasesLeft;
  // setLeftVars.insert(v);
  computeAliasesForRHS(bop.getLHS(), aliasesLeft, curInstr);
  // aliasTable.getModifiedVariables(setLeftVars,depth);
  if (isPointerQualType(bop.getLHS()->getType())) {
    handlePointersBinaryAssignment(bop, curInstr, mainVariable, declOrArrayExpr,
                                   aliasesLeft);
  }
  // It's an assignment, so all aliases are written
  for (auto &alias : aliasesLeft) {
    addDependencyWrite(curInstr, alias);
    if (isa<CompoundAssignOperator>(bop))
      addDependencyRead(curInstr, alias);
  }
  // If we have a pointer, then the aliases are related to the pointed
  // values when depth > 0 So we have to add a read dependency on the
  // pointer itself
  auto aliasMain =
      aliasTable.getOrAddAliasArg(declOrArrayExpr, getAliasType(bop.getLHS()));
  int depth =
      getPtrDepthAccess(declOrArrayExpr->getType(), bop.getLHS()->getType(),
                        mainVariable->getASTContext());

  if (depth > 0) {
    std::unordered_set<std::shared_ptr<aliasArg>> aliases;
    aliases.insert(aliasMain);
    aliasTable.getPointerAccessedVariables(aliases, depth);
    for (auto alias : aliases)
      addDependencyRead(curInstr, alias);
  }
  handleStmt(*bop.getRHS(), curInstr);
}
void ASTTaskGraphVisitor::handlePointersBinaryAssignment(
    const BinaryOperator &bop, Instruction &curInstr,
    const VarDecl *mainVariable, const Expr *declOrArrayExpr,
    std::unordered_set<std::shared_ptr<aliasArg>> &aliasesLeft) {

  // If a single pointer is aliased, then we know that its aliased
  // elements will change If there are multiple, it's because we can't be
  // sure which one is aliased, so we can't remove the dependencies
  // if (aliasesLeft.size() == 1)
  aliasTable.removeDependencyPtr(*aliasesLeft.begin());

  std::unordered_set<std::shared_ptr<aliasArg>> aliasesRHS;
  computeAliasesForRHS(bop.getRHS(), aliasesRHS, curInstr);
  const Expr *declOrArrayExprRHS = nullptr;
  if (declOrArrayExprRHS =
          getSingleArraySubscriptExprInsideExpr(bop.getRHS())) {
    ;
  } else if (declOrArrayExprRHS =
                 getSingleDeclRefExprInsideExpr(bop.getRHS())) {
    ;
  } else if (isa<CallExpr>(bop.getRHS()->IgnoreParenImpCasts())) {
    std::vector<const Expr *> declOrArrayExprRHSVect;

    auto rightCall = cast<CallExpr>(bop.getRHS()->IgnoreParenImpCasts());
    std::stack<const CallExpr *> callStack;
    callStack.push(rightCall);
    // We will check recursively all function arguments, and retrieve variables
    // that might be returned by the function (if it returns a pointer)
    while (!callStack.empty()) {
      auto curCall = callStack.top();
      auto rightFunction = curCall->getDirectCallee();
      callStack.pop();
      for (unsigned int i = 0; i < rightCall->getNumArgs(); i++) {
        if (isPointerQualType(rightFunction->getParamDecl(i)->getType())) {
          const Expr *arg = rightCall->getArg(i);
          if (isa<CallExpr>(arg->IgnoreParenImpCasts()))
            callStack.push(cast<CallExpr>(arg->IgnoreParenImpCasts()));
          else {
            auto depth =
                getPtrDepthAccess(mainVariable->getType(), arg->getType(),
                                  mainVariable->getASTContext());
            if (depth == 0) {
              for (auto &aliasLeft : aliasesLeft) {
                for (auto &aliasRight : aliasesRHS) {
                  aliasTable.addAliasPtr(aliasRight, aliasLeft);
                }
              }
            }
          }
        }
      }
    }
  }
  if (declOrArrayExprRHS != nullptr) {
    int depth = getPtrDepthAccess(declOrArrayExprRHS->getType(),
                                  bop.getRHS()->getType(),
                                  mainVariable->getASTContext());
    if (depth == -1) {
      for (auto &aliasLeft : aliasesLeft) {
        for (auto &aliasRight : aliasesRHS) {
          aliasTable.addAliasPtr(aliasRight, aliasLeft);
        }
      }
    } else
      for (auto &aliasLeft : aliasesLeft)
        for (auto &aliasRight : aliasesRHS)
          aliasTable.addAliasedToElement(aliasRight, aliasLeft);
  }
}
void ASTTaskGraphVisitor::handleMemberCallExpr(const CXXMemberCallExpr &c,
                                               Instruction &curInstr,
                                               bool isWrite) {
  Expr *obj = c.getImplicitObjectArgument();
  // Handle the object of the call
  if (isa<DeclRefExpr>(obj)) {
    const auto alias = aliasTable.getOrAddAliasArg(obj);
    // Alias is supposed to be read
    addDependencyRead(curInstr, alias);
    // If the method is not const, then we assume it may be written
    if (!c.getMethodDecl()->isConst())
      addDependencyWrite(curInstr, alias);
  } else
    handleStmt(*obj, curInstr);
  // Handle the call itself
  handleCallExpr(c, curInstr);
}
void ASTTaskGraphVisitor::handleCallExpr(const CallExpr &c,
                                         Instruction &curInstr, bool isWrite) {
  // TODO: Methods and read/write on object/data ?
  const FunctionDecl &f = *(c.getDirectCallee());
  // We look though each parameter of the function
  for (unsigned int i = 0; i < f.getNumParams(); i++) {
    // The parameter
    const ParmVarDecl &p = *(f.getParamDecl(i));
    // The corresponding argument
    const Expr *b = c.getArg(i);
    // If we have a variable, then there might be a write
    if (isInExceptionList(p)) {
      const Expr *d = b;
      if (isa<CXXBindTemporaryExpr>(d))
        d = cast<CXXBindTemporaryExpr>(*(b->IgnoreCasts())).getSubExpr();
      if (isa<CXXConstructExpr>(d))
        d = cast<CXXConstructExpr>(d)->getArg(0);
      if (isa<DeclRefExpr>(d->IgnoreParenImpCasts())) {
        handleStmt(*d, curInstr, true);
      }
    } else if (!(isFullConstType(p.getType())) &&
               (isReferenceQualType(p.getType()) ||
                isPointerQualType(p.getType()))) {
      // If the parameter can be modified (parameter is either a reference or
      // a pointer AND it's not completely const)
      //   then there might be a write, so we assume there is one
      const Expr *choosenExpr = b;
      if (getSingleArraySubscriptExprInsideExpr(b) != nullptr)
        choosenExpr = getSingleArraySubscriptExprInsideExpr(b);
      else if (getSingleDeclRefExprInsideExpr(b) != nullptr)
        choosenExpr = getSingleDeclRefExprInsideExpr(b);
      handleStmt(*choosenExpr, curInstr, true);
    }
    // Otherwise, we look through the expression since it is the same as
    // looking through any expression
    else
      handleStmt(*b, curInstr);
  }
}
void ASTTaskGraphVisitor::handleStmt(const Stmt &st, Instruction &instr,
                                     bool isWrite, bool isRead) {
  // Simple switch case to call the respective handle method, except for
  // DeclRefExpr which is a variable so it is a read
  // Dependency on return statement
  if (isa<ReturnStmt>(st)) {
    if (cast<ReturnStmt>(st).getRetValue())
      handleStmt(*(cast<ReturnStmt>(st).getRetValue()), instr, isWrite);
  }

  else if (isa<Expr>(st)) {

    auto curExp = (cast<Expr>(st).IgnoreParenImpCasts());
    if (isa<DeclRefExpr>(curExp)) {
      // const VarDecl *v = cast<VarDecl>(cast<DeclRefExpr>(curExp)->getDecl());
      auto alias = aliasTable.getOrAddAliasArg(curExp);
      if (isRead)
        addDependencyRead(instr, alias);
      if (isWrite)
        addDependencyWrite(instr, alias);
    } else {
      addDependenciesRead(instr, curExp);
      if (isa<CXXOperatorCallExpr>(curExp))
        handleCXXOperatorCallExpr(*cast<CXXOperatorCallExpr>(curExp), instr,
                                  isWrite);
      else if (isa<UnaryOperator>(curExp))
        handleUnaryOperator(*cast<UnaryOperator>(curExp), instr, isWrite);
      else if (isa<BinaryOperator>(curExp))
        handleBinaryOperator(*cast<BinaryOperator>(curExp), instr, isWrite);
      else if (isa<CXXMemberCallExpr>(curExp))
        handleMemberCallExpr(*cast<CXXMemberCallExpr>(curExp), instr, isWrite);
      else if (isa<CallExpr>(curExp))
        handleCallExpr(*cast<CallExpr>(curExp), instr, isWrite);
      else if (isa<ArraySubscriptExpr>(curExp)) {
        auto arrayExpr = getSingleArraySubscriptExprInsideExpr(curExp);
        if (arrayExpr) {
          auto baseExpr = getArrayBaseDeclRefExpr(arrayExpr);
          // auto base = cast<VarDecl>(baseExpr->getDecl());
          auto indexes =
              getArraySubscriptsIndexesValues(cast<ArraySubscriptExpr>(curExp));
          // We don't use the type of the base but the type of the expression
          // (tab will be a pointer but tab[0] might not be one)
          auto alias = aliasTable.getOrAddAliasArg(arrayExpr);
          if (isRead)
            addDependencyRead(instr, alias);
          if (isWrite)
            addDependencyWrite(instr, alias);
        }
        handleStmt(*cast<ArraySubscriptExpr>(curExp)->getIdx(), instr);
      }
      // Ignored expressions case
      else if (isa<IntegerLiteral>(curExp)) {
        ; // Do nothing
      } else {
        llvm::errs() << "Unhandled expression\n";
        llvm::errs() << TheRewriter.getSourceMgr()
                            .getPresumedLoc(curExp->getBeginLoc())
                            .getFilename()
                     << ":";
        curExp->dump();
      }
    }
  }
}

bool ASTTaskGraphVisitor::TraverseFunctionDecl(FunctionDecl *f) {
  if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc()))
    return true;
  if (f->getNameAsString().find("_apacSeq") != std::string::npos)
    return true;
  // If function is not in headers and has a body and is a definition, then we
  // traverse it recursively Using traverse we can avoid visiting nodes that
  // we don't need
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
  compInstr.noFusion = true;
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
  StmtOrder *outerInstrOrder = currentOrderManager;
  if (!ignoreStmtPragma) {
    currentOrderManager->addInstructionToManager(i);
  }
  if (currentOrderManager->getSubStmtOrder(i) != nullptr) {
    currentOrderManager = (outerInstrOrder->getSubStmtOrder(i)).get();
  }
  Instruction compInstr(i, getStmtAsString(i, TheRewriter.getLangOpts()), true,
                        0);
  compInstr.noFusion = true;
  handleStmt(*i->getCond(), compInstr);
  functionsInstructionsVector.push_back(std::vector<Instruction>());
  // res=RecursiveASTVisitor::TraverseIfStmt(i);
  if (i->getThen() && isa<CompoundStmt>(i->getThen())) {
    CompoundStmt *c = cast<CompoundStmt>(i->getThen());
    StmtOrder *outerInstrOrder2 = currentOrderManager;
    if (!ignoreStmtPragma) {
      currentOrderManager->addInstructionToManager(c);
    }
    if (currentOrderManager->getSubStmtOrder(c) != nullptr) {
      currentOrderManager = (outerInstrOrder2->getSubStmtOrder(c)).get();
    }
    Instruction compInstr(c, "if", true, 0);
    functionsInstructionsVector.push_back(std::vector<Instruction>());
    res = RecursiveASTVisitor::TraverseCompoundStmt(c);
    compInstr.scopedInstructions = functionsInstructionsVector.back();
    for (auto &instr : compInstr.scopedInstructions) {
      /*
      for (auto &dep : instr.dependencies) {
        compInstr.dependencies.insert(dep);
      }
      */
      if (instr.complexInstruction) {
        compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
      }
      compInstr.scopedInstructionsNumber++;
    }
    currentOrderManager = outerInstrOrder2;
    functionsInstructionsVector.pop_back();
    functionsInstructionsVector.back().push_back(compInstr);
  }

  if (i->getElse()) {
    if (isa<CompoundStmt>(i->getElse())) {

      CompoundStmt *c = cast<CompoundStmt>(i->getElse());
      StmtOrder *outerInstrOrder2 = currentOrderManager;
      if (!ignoreStmtPragma) {
        currentOrderManager->addInstructionToManager(c);
      }

      if (currentOrderManager->getSubStmtOrder(c) != nullptr) {
        currentOrderManager = (outerInstrOrder2->getSubStmtOrder(c)).get();
      }
      Instruction compInstr(c, "else", true, 0);

      functionsInstructionsVector.push_back(std::vector<Instruction>());
      res = RecursiveASTVisitor::TraverseCompoundStmt(c);
      compInstr.scopedInstructions = functionsInstructionsVector.back();
      for (auto &instr : compInstr.scopedInstructions) {
        /*
        for (auto &dep : instr.dependencies) {
          compInstr.dependencies.insert(dep);
        }
        */
        if (instr.complexInstruction) {
          compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
        }
        compInstr.scopedInstructionsNumber++;
      }
      currentOrderManager = outerInstrOrder2;
      functionsInstructionsVector.pop_back();
      functionsInstructionsVector.back().push_back(compInstr);
    } else {
      Instruction instr(i->getElse(), "else", true, 0);
      functionsInstructionsVector.push_back(std::vector<Instruction>());
      res = RecursiveASTVisitor::TraverseStmt(i->getElse());
      compInstr.scopedInstructions = functionsInstructionsVector.back();
      for (auto &instr : compInstr.scopedInstructions) {
        /*
        for (auto &dep : instr.dependencies) {
          compInstr.dependencies.insert(dep);
        }
        */
        if (instr.complexInstruction) {
          compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
        }
        compInstr.scopedInstructionsNumber++;
      }
      functionsInstructionsVector.pop_back();
      functionsInstructionsVector.back().push_back(compInstr);
    }
  }
  currentOrderManager = outerInstrOrder;
  compInstr.scopedInstructions = functionsInstructionsVector.back();
  for (auto &instr : compInstr.scopedInstructions) {
    /*
    for (auto &dep : instr.dependencies) {
      compInstr.dependencies.insert(dep);
    }
    */
    if (instr.complexInstruction) {
      compInstr.scopedInstructionsNumber += instr.scopedInstructions.size();
    }
    compInstr.scopedInstructionsNumber++;
  }
  functionsInstructionsVector.pop_back();
  functionsInstructionsVector.back().push_back(compInstr);
  return res;
}

const Expr *ASTTaskGraphVisitor::initVarFromExpr(const Expr *lhs,
                                                 const VarDecl *&mainVariable,
                                                 std::vector<int> &indexes) {
  const ArraySubscriptExpr *array = nullptr;
  const DeclRefExpr *d = nullptr;
  const Expr *declOrArrayExpr = nullptr;
  if ((array = getSingleArraySubscriptExprInsideExpr(lhs)) != nullptr) {
    const auto baseDeclExpr = getArrayBaseDeclRefExpr(array);
    mainVariable = cast<VarDecl>(baseDeclExpr->getDecl());
    indexes = getArraySubscriptsIndexesValues(array);
    declOrArrayExpr = array;
  }
  // Variable (not array)
  else if ((d = getSingleDeclRefExprInsideExpr(lhs))) {
    mainVariable = cast<VarDecl>(d->getDecl());
    declOrArrayExpr = d;
  }

  return declOrArrayExpr;
}