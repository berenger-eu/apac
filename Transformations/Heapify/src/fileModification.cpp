#include "fileModification.hpp"

void modifyFile(const std::unordered_map<CompoundStmt *,
                                         std::shared_ptr<ScopeInfo>> &scopes,
                Rewriter &TheRewriter) {
  for (auto &scope : scopes) {
    auto &scopeInfo = scope.second;
    if (scopeInfo->doesNotNeedHeap == 0) {
      // Create heaped variables
      for (auto &var : scopeInfo->variables) {
        auto item = item_found();
        initItem(item, *var);
        TheRewriter.ReplaceText(
            SourceRange(var->getBeginLoc(), var->getEndLoc()),
            createCreationString(item, TheRewriter.getLangOpts()));
      }
      // Create delete segment
      TheRewriter.InsertText(scope.first->getEndLoc(),
                             createDeleteSegment(scopeInfo->variables));
    }
  }
}

// Builds the string to delete a variable
std::string createDeleteString(VarDecl *var) {
  std::stringstream SSprint;
  SSprint << "delete ";
  if (isArrayVariable(*var)) {
    SSprint << "[] ";
  }
  SSprint << getApacMemBlockStr(var) << ";\n";
  return SSprint.str();
}

std::string createDeleteSegment(const std::vector<VarDecl *> &varsToDelete) {
  std::stringstream SSprint;
  // We iterate over all elements that have to be deleted
  // and create the corresponding text part
  for (auto var : varsToDelete) {
    SSprint << createDeleteString(var);
  }
  return SSprint.str();
}

std::string createCreationStringNonArray(const struct item_found &itFound,
                                         const LangOptions &langOpts) {
  std::stringstream SSprint;
  VarDecl &v = *(itFound.declaration);
  std::string strTempMemType = itFound.qTypeTempMem.getAsString(langOpts);
  std::string strNewType = itFound.qTypeNew.getAsString(langOpts);
  std::string strVarType = itFound.qTypeVar.getAsString(langOpts);
  std::string apacMemBloc = getApacMemBlockStr(itFound.declaration);

  SSprint << strTempMemType << ' ' << apacMemBloc << " = new " << strNewType;
  if (v.getInit() != NULL) {
    SSprint << '(' << getInitString(v) << ')';
  } else {
    SSprint << "()";
  }

  SSprint << ";\n"
          << strVarType << v.getNameAsString() << "= *(" << apacMemBloc
          << ");\n";
  return SSprint.str();
}
std::string createCreationStringArray(const struct item_found &itFound,
                                      const LangOptions &langOpts) {
  std::stringstream SSprint;
  VarDecl &v = *(itFound.declaration);
  std::string strTempMemType = itFound.qTypeTempMem.getAsString(langOpts);
  std::string strNewType = itFound.qTypeNew.getAsString(langOpts);
  std::string strVarType = itFound.qTypeVar.getAsString(langOpts);
  std::string apacMemBloc = getApacMemBlockStr(itFound.declaration);

  // strVarType adds a '&' before where the name of the variable should be
  // So using the its index, we can find where to place the variable name
  // in strTempMemType which is the same string as strVarType except for the '&'
  std::string strStart(strTempMemType);
  std::size_t found = strVarType.find('&');
  if (found != std::string::npos) {
    strStart.insert(found, ' ' + apacMemBloc);
  }

  SSprint << strStart << " = new " << strNewType;
  if (v.getInit() != NULL) {
    SSprint << getInitString(v);
  }

  strStart = strVarType;
  if (found != std::string::npos) {
    strStart.insert(found + 1, itFound.name);
  }

  SSprint << ";\n" << strStart << "= (" << apacMemBloc << ");\n";
  return SSprint.str();
}

void initItem(struct item_found &item, VarDecl &vDec) {
  item.name = vDec.getNameAsString();
  item.array = isArrayVariable(vDec);
  item.found = true;
  item.declaration = &vDec;
  item.qTypeNew = vDec.getType();
  if (vDec.getType().getTypePtrOrNull()->isReferenceType() ||
      !isInitAReference(vDec)) {
    item.qTypeNew = getUnreferencedQType(item.qTypeNew, vDec.getASTContext());
  }
  if (item.array) {

    item.qTypeTempMem =
        vDec.getASTContext().getPointerType(vDec.getType()
                                                .getTypePtrOrNull()
                                                ->getAsArrayTypeUnsafe()
                                                ->getElementType());
    item.qTypeVar =
        getReferenceToQType(item.qTypeTempMem, vDec.getASTContext());
  } else {

    item.qTypeTempMem = vDec.getASTContext().getPointerType(item.qTypeNew);
    item.qTypeTempMem.addConst();
    item.qTypeVar = getReferenceToQType(item.qTypeNew, vDec.getASTContext());
  }
}
