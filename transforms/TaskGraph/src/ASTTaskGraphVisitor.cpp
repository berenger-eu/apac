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
    if (!f) {
      handleCallExpr(*c, instr);
      return;
    }
    for (unsigned int i = 0; i < std::min(c->getNumArgs(), f->getNumParams());
         i++) {
      const ParmVarDecl *curParam = f->getParamDecl(i);
      if ((isPointerQualType(curParam->getType()) ||
           isReferenceQualType(curParam->getType())) &&
          !isFullConstType(curParam->getType())) {
        const Expr *arg = c->getArg(i);
        std::unordered_set<std::shared_ptr<aliasArg>> subAliases;
        computeAliasesForRHS(arg, subAliases, instr);
        for (auto &alias : subAliases)
          aliases.insert(alias);
        const auto *argDecl = getSingleDeclRefExprInsideExpr(arg);
        if (argDecl && isa<VarDecl>(argDecl->getDecl())) {
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
      const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(c.getArg(1));
      const ArraySubscriptExpr *array =
          getSingleArraySubscriptExprInsideExpr(c.getArg(1));
      std::vector<const Expr *> declOrArray;
      if (array != nullptr) {
        // const auto baseDeclExpr =
        //     getSingleDeclRefExprInsideExpr(getArrayBaseDeclRefExpr(array));
        //  v2 = cast<VarDecl>(baseDeclExpr->getDecl());
        indexes = getArraySubscriptsIndexesValues(array);
        declOrArray.push_back(array);

      } else if (d != nullptr) {
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
          if (!functionCalled) {
            handleCallExpr(*rightCall, instr);
            return;
          }
          getLowestType(arguments, types, functionCalled->getASTContext());
          for (long unsigned int i = 0;
               i < std::min(arguments.size(),
                            size_t(functionCalled->getNumParams()));
               i++) {
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
  // A function pointer has mutable pointer storage, but its target is code,
  // not a data object whose aliases belong in the pointee table.
  if (isPointerQualType(bop.getLHS()->getType()) &&
      !bop.getLHS()->getType()->isFunctionPointerType()) {
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

  std::unordered_set<std::shared_ptr<aliasArg>> aliasesRHS;
  computeAliasesForRHS(bop.getRHS(), aliasesRHS, curInstr);
  const Expr *declOrArrayExprRHS = nullptr;
  if (getSingleArraySubscriptExprInsideExpr(bop.getRHS()) != nullptr) {
    declOrArrayExprRHS = getSingleArraySubscriptExprInsideExpr(bop.getRHS());

  } else if (getSingleDeclRefExprInsideExpr(bop.getRHS()) != nullptr) {
    declOrArrayExprRHS = getSingleDeclRefExprInsideExpr(bop.getRHS());
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
      if (!rightFunction)
        continue;
      for (unsigned int i = 0;
           i < std::min(curCall->getNumArgs(), rightFunction->getNumParams());
           i++) {
        if (isPointerQualType(rightFunction->getParamDecl(i)->getType())) {
          const Expr *arg = curCall->getArg(i);
          if (isa<CallExpr>(arg->IgnoreParenImpCasts()))
            callStack.push(cast<CallExpr>(arg->IgnoreParenImpCasts()));
          else {
            auto depth =
                getPtrDepthAccess(arg->getType(), bop.getLHS()->getType(),
                                  mainVariable->getASTContext());
            // So the argument is either :
            //  - the same type as the LHS
            //  - or a pointer that points to the same type as the LHS at some
            //  point

            // retrieve aliases, use depth to get modified variables, fuse
            if (depth >= 0) {
              std::unordered_set<std::shared_ptr<aliasArg>> aliasesRHS;
              computeAliasesForRHS(arg, aliasesRHS, curInstr);
              auto argVar = aliasTable.getOrAddAliasArg(
                  getSingleDeclRefExprInsideExpr(arg));
              assert(argVar != nullptr);
              if (argVar != nullptr &&
                  getPtrDepthAccess(arg->getType(),
                                    argVar->declaration.getType(),
                                    mainVariable->getASTContext()) == 1) {
                for (auto &aliasLeft : aliasesLeft)
                  for (auto &aliasRight : aliasesRHS)
                    aliasTable.addAliasPtr(aliasRight, aliasLeft);
              } else {

                aliasTable.getModifiedVariables(aliasesRHS, depth);
                for (auto &aliasLeft : aliasesLeft) {
                  for (auto &aliasRight : aliasesRHS) {
                    aliasTable.fuseAliased(aliasRight, aliasLeft);
                  }
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
      aliasTable.removeDependencyPtr(*aliasesLeft.begin());
      for (auto &aliasLeft : aliasesLeft) {
        for (auto &aliasRight : aliasesRHS) {
          aliasTable.addAliasPtr(aliasRight, aliasLeft);
        }
      }
    } else
      for (auto &aliasLeft : aliasesLeft)
        for (auto &aliasRight : aliasesRHS)
          aliasTable.replaceAliasedToElement(aliasRight, aliasLeft);
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
  const auto *callee = c.getDirectCallee();
  if (!callee) {
    handleStmt(*c.getCallee(), curInstr);
    for (const auto *arg : c.arguments())
      handleStmt(*arg, curInstr, true);
    return;
  }
  // Only explicit arguments contribute dependencies here. Globals accessed
  // inside the callee are not propagated to the caller; their synchronization
  // belongs to the program. Global arguments use the ordinary alias analysis.
  const FunctionDecl &f = *callee;
  // Variadic arguments can also carry addresses of mutable data.
  for (unsigned i = f.getNumParams(); i < c.getNumArgs(); ++i)
    handleStmt(*c.getArg(i), curInstr, true);
  for (unsigned int i = 0; i < std::min(f.getNumParams(), c.getNumArgs());
       i++) {
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
      // If the parameter can be modified (parameter is either a reference
      // or a pointer AND it's not completely const)
      //   Check interprocedural analysis: is the param actually read-only?
      bool isActuallyWritten = true;
      if (paramAnalyzer_ &&
          paramAnalyzer_->isParamReadOnly(&f, i)) {
        isActuallyWritten = false;
      }
      const Expr *choosenExpr = b;
      if (getSingleArraySubscriptExprInsideExpr(b) != nullptr)
        choosenExpr = getSingleArraySubscriptExprInsideExpr(b);
      else if (getSingleDeclRefExprInsideExpr(b) != nullptr)
        choosenExpr = getSingleDeclRefExprInsideExpr(b);
      handleStmt(*choosenExpr, curInstr, isActuallyWritten);
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
      // Function symbols (including &function in callback assignments) do not
      // denote mutable variable storage and cannot be represented by aliasArg.
      if (!isa<VarDecl>(cast<DeclRefExpr>(curExp)->getDecl()))
        return;
      // const VarDecl *v =
      // cast<VarDecl>(cast<DeclRefExpr>(curExp)->getDecl());
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
          // auto base = cast<VarDecl>(baseExpr->getDecl());
          auto indexes =
              getArraySubscriptsIndexesValues(cast<ArraySubscriptExpr>(curExp));
          // We don't use the type of the base but the type of the
          // expression (tab will be a pointer but tab[0] might not be one)
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
  if (functionsConditions(f)) {
    functionsInstructionsVector.push_back(std::vector<Instruction>());
    bool previous = sequentialTail_;
    auto previousToken = arrayPhaseToken_;
    auto functionIndex = functionsInstructionsVector.size() - 1;
    arrayPhaseToken_ = "__apac_array_phase_" + std::to_string(functionIndex);
    sequentialTail_ = false;
    bool result = RecursiveASTVisitor::TraverseFunctionDecl(f);
    if (finishArrayPhases(functionsInstructionsVector[functionIndex]))
      arrayPhaseTokens_[f] = arrayPhaseToken_;
    arrayPhaseToken_ = previousToken;
    sequentialTail_ = previous;
    return result;
  }
  return true;
}

bool ASTTaskGraphVisitor::preservesAliases(const Stmt *root) const {
  // This deliberately proves only a scalar subset. Pointer/reference/object
  // operations, inline assembly and calls without a visible body may change
  // aliases; do not resume tasking using stale alias information in those cases.
  auto scalarType = [](QualType type) {
    return type->isArithmeticType() || type->isEnumeralType() ||
           type->isVoidType() || type->isFunctionType();
  };
  auto scalarVariable = [&](const VarDecl *variable) {
    // TLS initialization and cleanup attributes may invoke code implicitly,
    // even when the declared value has a scalar type.
    return scalarType(variable->getType()) &&
           variable->getTLSKind() == VarDecl::TLS_None && !variable->hasAttrs();
  };
  std::vector<const Stmt *> pending{root};
  std::unordered_set<const FunctionDecl *> visited;
  while (!pending.empty()) {
    const Stmt *statement = pending.back();
    pending.pop_back();
    if (isa<AsmStmt>(statement) || isa<CXXConstructExpr>(statement))
      return false;
    if (const auto *expr = dyn_cast<Expr>(statement)) {
      const auto *cast = dyn_cast<ImplicitCastExpr>(expr);
      // Direct calls contain a function-to-pointer decay even when no mutable
      // pointer is involved. Indirect calls are rejected below.
      bool directFunctionDecay =
          cast && cast->getCastKind() == CK_FunctionToPointerDecay;
      if (!directFunctionDecay && !scalarType(expr->getType()))
        return false;
    }
    if (const auto *ref = dyn_cast<DeclRefExpr>(statement))
      if (const auto *variable = dyn_cast<VarDecl>(ref->getDecl()))
        if (!scalarVariable(variable))
          return false;
    if (const auto *declarations = dyn_cast<DeclStmt>(statement))
      for (const auto *declaration : declarations->decls())
        if (const auto *variable = dyn_cast<VarDecl>(declaration))
          if (!scalarVariable(variable))
            return false;
    if (const auto *call = dyn_cast<CallExpr>(statement)) {
      const auto *callee = call->getDirectCallee();
      if (!callee || !callee->getBody())
        return false;
      if (visited.insert(callee->getCanonicalDecl()).second)
        pending.push_back(callee->getBody());
    }
    for (const auto *child : statement->children())
      if (child)
        pending.push_back(child);
  }
  return true;
}

bool ASTTaskGraphVisitor::traverseSynchronous(Stmt *s) {
  if (isInHeaders(TheRewriter.getSourceMgr(), s->getBeginLoc()))
    return true;
  // Calls to this helper after an uncertain loop must never clear the fallback.
  sequentialTail_ = sequentialTail_ || !preservesAliases(s);
  currentOrderManager->addInstructionToManager(s, true);
  Instruction instr(s, getStmtAsStringFull(s, TheRewriter.getLangOpts()));
  instr.noFusion = true;
  instr.synchronous = true;
  functionsInstructionsVector.back().push_back(instr);
  // Preserve control flow as a whole: the header, break/continue and nested
  // declarations must not be rewritten as independent OpenMP tasks.
  return true;
}

bool ASTTaskGraphVisitor::TraverseForStmt(ForStmt *f) {
  if (!sequentialTail_ && !isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
    std::string replacement;
    if (lowerPartitionCallLoop(f, aliasTable, paramAnalyzer_, replacement)) {
      currentOrderManager->addInstructionToManager(f, true);
      Instruction instruction(f, getStmtAsStringFull(f, TheRewriter.getLangOpts()));
      instruction.synchronous = true;
      instruction.noFusion = true;
      instruction.loopTaskReplacement = replacement;
      functionsInstructionsVector.back().push_back(instruction);
      return true;
    }
  }
  return traverseSynchronous(f);
}
// TODO: refacto
bool ASTTaskGraphVisitor::TraverseIfStmt(IfStmt *i) {
  if (sequentialTail_ ||
      (i->getThen() && !isa<CompoundStmt>(i->getThen())) ||
      (i->getElse() && !isa<CompoundStmt>(i->getElse())))
    return traverseSynchronous(i);
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
  const ArraySubscriptExpr *array = getSingleArraySubscriptExprInsideExpr(lhs);
  const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(lhs);
  const Expr *declOrArrayExpr = nullptr;
  if (array != nullptr) {
    const auto baseDeclExpr = getArrayBaseDeclRefExpr(array);
    mainVariable = cast<VarDecl>(baseDeclExpr->getDecl());
    indexes = getArraySubscriptsIndexesValues(array);
    declOrArrayExpr = array;
  }
  // Variable (not array)
  else if (d != nullptr) {
    mainVariable = cast<VarDecl>(d->getDecl());
    declOrArrayExpr = d;
  }

  return declOrArrayExpr;
}