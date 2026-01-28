#include "fileModification.hpp"

void modifyFile(const std::unordered_map<CompoundStmt *,
                                         std::shared_ptr<ScopeInfo>> &scopes,
                Rewriter &TheRewriter) {
  for (auto &scope : scopes) {
    auto &scopeInfo = scope.second;
    if (scopeInfo->doesNotNeedHeap == 0) {
      // Create heaped variables
      for (auto &itemHeap : scopeInfo->variablesToHeap) {
        auto &var = itemHeap->declaration;
        TheRewriter.ReplaceText(
            SourceRange(var->getBeginLoc(), var->getEndLoc()),
            createCreationString(itemHeap, TheRewriter.getLangOpts()));
      }
      // Create delete segment

      TheRewriter.InsertText(scope.first->getEndLoc(),
                             createDeleteSegment(scopeInfo->variablesToHeap));
    }
    for (auto &gotoRetStmt : scopeInfo->goToReturnStmts) {
      llvm::errs() << "test\n";
      llvm::errs() << scopeInfo->itemsToDelete.size() << "\n";
      TheRewriter.InsertText(gotoRetStmt->getBeginLoc(),
                             createDeleteSegment(scopeInfo->itemsToDelete));
    }
  }
}
// Builds the string to delete a variable
std::string createDeleteString(const std::shared_ptr<struct item_found> &var) {
  std::stringstream SSprint;
  SSprint << "delete ";
  if (var->array) {
    SSprint << "[] ";
  }
  SSprint << getApacMemBlockStr(var) << ";\n";
  return SSprint.str();
}

std::string createDeleteSegment(
    const std::vector<std::shared_ptr<struct item_found>> &varsToDelete) {
  std::stringstream SSprint;
  // We iterate over all elements that have to be deleted
  // and create the corresponding text part
  for (auto &var : varsToDelete) {
    SSprint << createDeleteString(var);
  }
  return SSprint.str();
}

std::string
createCreationStringNonArray(const std::shared_ptr<struct item_found> &itFound,
                             const LangOptions &langOpts) {
  std::stringstream SSprint;
  VarDecl *v = (itFound->declaration);
  std::string strTempMemType = itFound->qTypeTempMem.getAsString(langOpts);
  std::string strNewType = itFound->qTypeNew.getAsString(langOpts);
  std::string strVarType = itFound->qTypeVar.getAsString(langOpts);
  std::string apacMemBloc = getApacMemBlockStr(itFound);

  SSprint << strTempMemType << ' ' << apacMemBloc << " = new " << strNewType;
  if (v->getInit() != NULL) {
    SSprint << '(' << getInitString(v) << ')';
  } else {
    SSprint << "()";
  }

  SSprint << ";\n"
          << strVarType << v->getNameAsString() << "= *(" << apacMemBloc
          << ");\n";
  return SSprint.str();
}
std::string
createCreationStringArray(const std::shared_ptr<struct item_found> &itFound,
                          const LangOptions &langOpts) {
  std::stringstream SSprint;
  VarDecl *v = (itFound->declaration);
  std::string strTempMemType = itFound->qTypeTempMem.getAsString(langOpts);
  std::string strNewType = itFound->qTypeNew.getAsString(langOpts);
  std::string strVarType = itFound->qTypeVar.getAsString(langOpts);
  std::string apacMemBloc = getApacMemBlockStr(itFound);

  // strVarType adds a '&' before where the name of the variable should be
  // So using the its index, we can find where to place the variable name
  // in strTempMemType which is the same string as strVarType except for the
  // '&'
  std::string strStart(strTempMemType);
  std::size_t found = strVarType.find('&');
  if (found != std::string::npos) {
    strStart.insert(found, ' ' + apacMemBloc);
  }

  SSprint << strStart << " = new " << strNewType;
  if (v->getInit() != NULL) {
    SSprint << getInitString(v);
  }

  strStart = strVarType;
  if (found != std::string::npos) {
    strStart.insert(found + 1, itFound->name);
  }

  SSprint << ";\n" << strStart << "= (" << apacMemBloc << ");\n";
  return SSprint.str();
}
