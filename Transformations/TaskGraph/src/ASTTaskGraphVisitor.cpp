#include "ASTTaskGraphVisitor.hpp"

void ASTTaskGraphVisitor::computeAliasesForRHS(
    const Expr *expression, std::unordered_set<const VarDecl *> &aliases,
    Instruction &instr) {
  int depth;
  const Expr *rhs = expression->IgnoreParenImpCasts();
  const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(rhs);
  // We look for variables aliased in a given expression
  // If we access a variable, then we found the aliases of the variable really
  // accessed
  //(*p), the variable is p, but in reality we access *p, so we need to look for
  //the aliases of *p
  if (d) {
    // Aliases will be values that refer (or might refer) to the same memory
    // location
    // TODO:Modif ici pour ajout alias aux nodes
    const VarDecl *v = cast<VarDecl>(d->getDecl());
    aliases.insert(v);
    depth = getPtrDepthAccess(*v, *rhs);
    // We retrive the pointed values or the references
    aliasTable.getModifiedVariables(aliases, depth);
    for (auto &ali : aliases) {
      if (v != ali) {
        instr.curAliases.insert({v, ali});
      }
    }
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
        std::unordered_set<const VarDecl *> subAliases;
        computeAliasesForRHS(arg, subAliases, instr);
        for (auto &alias : subAliases) {
          aliases.insert(alias);
        }
        if (isa<VarDecl>(getSingleDeclRefExprInsideExpr(arg)->getDecl())) {
          const VarDecl *v =
              cast<VarDecl>(getSingleDeclRefExprInsideExpr(arg)->getDecl());
          if (getPtrDepthAccess(*v, *arg) == -1) {
            aliases.insert(v);
          }
        }
      }
    }
    // TODO: Handle CallExpr
  } else {
    handleStmt(*rhs, instr);
  }
}

void ASTTaskGraphVisitor::handleCXXOperatorCallExpr(
    const CXXOperatorCallExpr &c, Instruction &instr, bool isWrite) {
  if (c.isAssignmentOp() && c.getNumArgs() == 2) {

    if (isa<DeclRefExpr>(c.getArg(0)) &&
        isReferenceQualType(c.getArg(0)->getType())) {
      const VarDecl *v =
          cast<VarDecl>(cast<DeclRefExpr>(c.getArg(0))->getDecl());
      const DeclRefExpr *d;
      if ((d = getSingleDeclRefExprInsideExpr(c.getArg(1))) != nullptr) {
        const VarDecl *v2 = cast<VarDecl>(d->getDecl());
        if (v2) {
          aliasTable.addAliasReference(v2, v);
          addDependencyRead(instr, v2);
          addDependencyWrite(instr, v);
        }
      }
    }
  } else {
    for (const auto &arg : c.arguments()) {
      handleStmt(*arg, instr, isWrite);
    }
  }
}

void ASTTaskGraphVisitor::handleUnaryOperator(const UnaryOperator &uop,
                                              Instruction &curInstr,
                                              bool isWrite) {
  Expr *subExpr = uop.getSubExpr();
  // If we have a variable
  const DeclRefExpr *d;
  if ((d = getSingleDeclRefExprInsideExpr(subExpr)) != nullptr) {
    // and we increment or decrement it, then it's a read and a write
    int depth;
    const VarDecl *v = cast<VarDecl>(d->getDecl());
    std::unordered_set<const VarDecl *> aliases;

    computeAliasesForRHS(&uop, aliases, curInstr);
    for (auto &alias : aliases) {
      addDependencyRead(curInstr, alias);
    }

    // TODO: check if other cases are read and/or write
    if (uop.isIncrementOp() || uop.isDecrementOp()) {
      for (auto &alias : aliases) {
        addDependencyWrite(curInstr, alias);
      }
    }
    if ((getPtrDepthAccess(*v, *subExpr)) > 0) {
      addDependencyRead(curInstr, v);
    }
  }
  // Otherwise, unary expression affects a temporary value so we ignore it but
  // still look through the expression
  else {
    handleStmt(*subExpr, curInstr);
  }
}

void ASTTaskGraphVisitor::handleBinaryOperator(const BinaryOperator &bop,
                                               Instruction &curInstr,
                                               bool isWrite) {
  // Special case for assignment operators, because it is a write
  if (bop.isAssignmentOp()) {
    // Most likely unnecessary since left side has to be a lvalue because of the
    // assignment operator
    const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(bop.getLHS());
    if (d) {
      const VarDecl *v = cast<VarDecl>(d->getDecl());
      std::unordered_set<const VarDecl *> aliases;
      // setLeftVars.insert(v);
      computeAliasesForRHS(bop.getLHS(), aliases, curInstr);
      // aliasTable.getModifiedVariables(setLeftVars,depth);
      if (isPointerQualType(bop.getLHS()->getType())) {
        // If a single pointer is aliased, then we know that its aliased
        // elements will change If there are multiple, it's because we can't be
        // sure which one is aliased, so we can't remove the dependencies

        if (aliases.size() == 1) {
          aliasTable.removeDependencyPtr(*aliases.begin());
        }

        std::unordered_set<const VarDecl *> aliasesRHS;
        computeAliasesForRHS(bop.getRHS(), aliasesRHS, curInstr);
        llvm::errs() << "Size of aliases: " << aliasesRHS.size()
                     << aliases.size() << "\n";
        for (auto &alias : aliasesRHS) {
          alias->dump();
        }
        const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(bop.getRHS());
        if (d) {
          if (getPtrDepthAccess(*cast<VarDecl>(d->getDecl()), *bop.getRHS()) ==
              -1) {
            for (auto &aliasLeft : aliases) {
              aliasTable.addAliasPtr(cast<VarDecl>(d->getDecl()), aliasLeft);
            }
          }
        }
        for (auto &aliasLeft : aliases) {
          for (auto &alias : aliasesRHS) {
            aliasTable.addAliasPtr(alias, aliasLeft);
          }
        }
      }
      for (auto &alias : aliases) {
        addDependencyWrite(curInstr, alias);
        if (isa<CompoundAssignOperator>(bop)) {
          addDependencyRead(curInstr, alias);
        }
      }
      int depth = getPtrDepthAccess(*v, *bop.getLHS());
      if (depth > 0) {
        addDependencyRead(curInstr, v);
      }

    } else {
      handleStmt(*bop.getLHS(), curInstr, true);
    }
    handleStmt(*bop.getRHS(), curInstr);
  }
  // Otherwise, we just look through the expression on both sides
  else {
    handleStmt(*bop.getLHS(), curInstr);
    handleStmt(*bop.getRHS(), curInstr);
  }
}
void ASTTaskGraphVisitor::handleMemberCallExpr(const CXXMemberCallExpr &c,
                                               Instruction &curInstr,
                                               bool isWrite) {
  Expr *obj = c.getImplicitObjectArgument();
  if (isa<DeclRefExpr>(obj)) {
    VarDecl *v = cast<VarDecl>(cast<DeclRefExpr>(obj)->getDecl());
    addDependencyRead(curInstr, v);
    if (!c.getMethodDecl()->isConst()) {
      addDependencyWrite(curInstr, v);
    }
  } else {
    handleStmt(*obj, curInstr);
  }
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
      if (isa<CXXBindTemporaryExpr>(d)) {
        d = cast<CXXBindTemporaryExpr>(*(b->IgnoreCasts())).getSubExpr();
      }
      if (isa<CXXConstructExpr>(d)) {
        d = cast<CXXConstructExpr>(d)->getArg(0);
      }
      if (isa<DeclRefExpr>(d->IgnoreParenImpCasts())) {
        const VarDecl *v = cast<VarDecl>(
            cast<DeclRefExpr>(d->IgnoreParenImpCasts())->getDecl());
        addDependencyRead(curInstr, v);
        addDependencyWrite(curInstr, v);
      }
    } else if (!(isFullConstType(p.getType()) &&
                 isReferenceQualType(p.getType())) ||
               isPointerQualType(p.getType())) {
      const DeclRefExpr *d = getSingleDeclRefExprInsideExpr(b);
      // If the parameter can be modified (parameter is either a reference or a
      // pointer AND it's not completely const)
      //   then there might be a write, so we assume there is one
      if (d) {
        const VarDecl *v = cast<VarDecl>(d->getDecl());
        addDependencyRead(curInstr, v);
        addDependencyWrite(curInstr, v);
        std::unordered_set<const VarDecl *> aliases(aliasTable.getAliased(v));
        for (auto &ali : aliases) {
          addDependencyRead(curInstr, ali);
          addDependencyWrite(curInstr, ali);
        }
      } else {
        llvm::errs() << "Failed to find DeclRefExpr\n";
      }
    }
    // Otherwise, we look through the expression since it is the same as looking
    // through any expression
    else {
      handleStmt(*b, curInstr);
    }
  }
}
void ASTTaskGraphVisitor::handleStmt(const Stmt &st, Instruction &instr,
                                     bool isWrite) {
  // Simple switch case to call the respective handle method, except for
  // DeclRefExpr which is a variable so it is a read
  if (isa<ReturnStmt>(st)) {
    if (cast<ReturnStmt>(st).getRetValue()) {
      handleStmt(*(cast<ReturnStmt>(st).getRetValue()), instr, isWrite);
    }
  } else if (isa<Expr>(st)) {
    const Expr &curExp = *(cast<Expr>(st).IgnoreParenImpCasts());
    if (isa<CXXOperatorCallExpr>(curExp)) {
      handleCXXOperatorCallExpr(cast<CXXOperatorCallExpr>(curExp), instr,
                                isWrite);
    } else if (isa<UnaryOperator>(curExp)) {
      handleUnaryOperator(cast<UnaryOperator>(curExp), instr, isWrite);
    } else if (isa<BinaryOperator>(curExp)) {
      handleBinaryOperator(cast<BinaryOperator>(curExp), instr, isWrite);
    } else if (isa<CXXMemberCallExpr>(curExp)) {
      handleMemberCallExpr(cast<CXXMemberCallExpr>(curExp), instr, isWrite);
    } else if (isa<CallExpr>(curExp)) {
      handleCallExpr(cast<CallExpr>(curExp), instr, isWrite);
    } else if (isa<DeclRefExpr>(curExp)) {
      const VarDecl *v = cast<VarDecl>(cast<DeclRefExpr>(curExp).getDecl());
      addDependencyRead(instr, v);
      if (isWrite) {
        addDependencyWrite(instr, v);
      }
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
  if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
    return true;
  }

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
  if (isInHeaders(TheRewriter.getSourceMgr(), f->getBeginLoc())) {
    return true;
  }
  bool res = true;
  Instruction compInstr{f, getStmtAsString(f, TheRewriter.getLangOpts()), true,
                        0};
  functionsInstructionsVector.push_back(std::vector<Instruction>());
  res = RecursiveASTVisitor::TraverseStmt(f->getCond());
  if (!res) {
    return false;
  }
  res = RecursiveASTVisitor::TraverseStmt(f->getBody());
  if (!res) {
    return false;
  }
  res = RecursiveASTVisitor::TraverseStmt(f->getInc());
  if (!res) {
    return false;
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

bool ASTTaskGraphVisitor::TraverseIfStmt(IfStmt *i) {
  if (isInHeaders(TheRewriter.getSourceMgr(), i->getBeginLoc())) {
    return true;
  }
  bool res = true;
  Instruction compInstr{i, getStmtAsString(i, TheRewriter.getLangOpts()), true,
                        0};
  functionsInstructionsVector.push_back(std::vector<Instruction>());
  // res=RecursiveASTVisitor::TraverseIfStmt(i);
  if (i->getThen() && isa<CompoundStmt>(i->getThen())) {
    CompoundStmt *c = cast<CompoundStmt>(i->getThen());
    Instruction compInstr;
    compInstr.instruction = c;
    std::stringstream ss;
    ss << "if";

    compInstr.instructionString = ss.str();
    compInstr.complexInstruction = true;
    compInstr.scopedInstructionsNumber = 0;
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
      Instruction compInstr;
      compInstr.instruction = c;
      std::stringstream ss;
      ss << "else";

      compInstr.instructionString = ss.str();
      compInstr.complexInstruction = true;
      compInstr.scopedInstructionsNumber = 0;

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
      Instruction instr;
      instr.instruction = i->getElse();
      instr.instructionString = "else";
      compInstr.complexInstruction = true;
      compInstr.scopedInstructionsNumber = 0;
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