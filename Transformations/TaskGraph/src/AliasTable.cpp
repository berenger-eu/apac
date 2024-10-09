#include "AliasTable.hpp"

using namespace clang;

std::unordered_set<std::shared_ptr<aliasArg>>
AliasTable::getAliased(std::shared_ptr<aliasArg> &v) {
  std::unordered_set<std::shared_ptr<aliasArg>> aliases;
  std::stack<std::shared_ptr<aliasArg>> aliasStack;
  if (v != nullptr)
    aliasStack.push(v);
  while (!aliasStack.empty()) {
    const auto cur = aliasStack.top();
    aliasStack.pop();
    if (aliases.count(cur) != 0)
      continue;
    aliases.insert(cur);
    if (cur->type == Reference || cur->type == Pointer)
      for (const auto &alias : cur->aliased)
        aliasStack.push(alias);
  }

  return aliases;
}

void AliasTable::addAliasReference(std::shared_ptr<aliasArg> &var,
                                   std::shared_ptr<aliasArg> &ref) {
  llvm::errs() << "Adding alias reference\n";
  if (var != nullptr && ref != nullptr) {

    llvm::errs() << "Got alias args\n";
    // TODO: move in different function
    /*
    if (tableValueRef == nullptr) {
      std::vector<int> keyIndexes;
      addElementToAliasTable(ref, AliasType::Reference, keyIndexes);
      llvm::errs() << "Inserted alias arg ref\n";
      tableValueRef = getAliasArg(ref);
    }
    llvm::errs() << "Got alias arg ref\n";
    if (tableValueVar == nullptr) {
      std::vector<int> keyIndexes;
      addElementToAliasTable(var, AliasType::Variable, keyIndexes);
      tableValueVar = getAliasArg(var);
    }
    */
    llvm::errs() << "Got alias arg var\n";
    var->references.insert(ref);
    ref->aliased.insert(var);
  }
  llvm::errs() << "Done adding alias reference\n";
}
void AliasTable::addAliasPtr(std::shared_ptr<aliasArg> var,
                             std::shared_ptr<aliasArg> ptr) {
  if (var != nullptr && ptr != nullptr) {
    var->pointers.insert(ptr);
    ptr->aliased.insert(var);
  }
}
void AliasTable::addAliasedToElement(std::shared_ptr<aliasArg> toAddAliaser,
                                     std::shared_ptr<aliasArg> targetAlias) {
  for (auto &aliasedElem : toAddAliaser->aliased) {
    targetAlias->aliased.insert(aliasedElem);
    aliasedElem->pointers.insert(targetAlias);
  }
}

/*
void AliasTable::addAliasPtr(const Expr *var, const Expr *ptr) {
  if (var != nullptr && ptr != nullptr) {
    std::vector<int> keyIndexesVar, keyIndexesPtr;
    const VarDecl *varDecl = nullptr, *ptrDecl = nullptr;
    if (isa<ArraySubscriptExpr>(var)) {
      const ArraySubscriptExpr *ase = cast<ArraySubscriptExpr>(var);
      if (isa<DeclRefExpr>(ase->getBase())) {
        varDecl = cast<VarDecl>(cast<DeclRefExpr>(ase->getBase())->getDecl());
      }
      keyIndexesVar = getArraySubscriptsIndexesValues(ase->getIdx());
    }
    if (isa<ArraySubscriptExpr>(ptr)) {
      const ArraySubscriptExpr *ase = cast<ArraySubscriptExpr>(ptr);
      if (isa<DeclRefExpr>(ase->getBase())) {
        ptrDecl = cast<VarDecl>(cast<DeclRefExpr>(ase->getBase())->getDecl());
      }
      keyIndexesPtr = getArraySubscriptsIndexesValues(ase->getIdx());
    }
    if (varDecl != nullptr && ptrDecl != nullptr) {
      addAliasPtr(varDecl, keyIndexesVar, ptrDecl, keyIndexesPtr,
                  getPtrDepthAccess(var->getType(), ptr->getType(),
                                    varDecl->getASTContext()));
    }
  }
  llvm::errs() << "Done adding alias ptr\n";
}
void AliasTable::addAliasPtr(const VarDecl *var,
                             const std::vector<int> &indexesVar,
                             const VarDecl *ptr,
                             const std::vector<int> &indexesPtr,
                             const int &depth) {
  if (var != nullptr && ptr != nullptr) {
    auto tableValuePtr = getAliasArg(ptr, indexesPtr);
    auto tableValueVar = getAliasArg(var, indexesVar);
    llvm::errs() << "Got alias args\n";
    if (tableValuePtr == nullptr) {
      addElementToAliasTable(ptr, AliasType::Pointer, indexesPtr);
      tableValuePtr = getAliasArg(ptr, indexesPtr);
    }
    if (isPointerQualType(var->getType())) {
      if (tableValueVar == nullptr) {
        addElementToAliasTable(var, AliasType::Pointer, indexesVar);
        tableValueVar = getAliasArg(var, indexesVar);
      }
    } else {
      if (tableValueVar == nullptr) {
        addElementToAliasTable(var, AliasType::Variable, indexesVar);
        tableValueVar = getAliasArg(var, indexesVar);
      }
    }
    llvm::errs() << "Got alias arg ptr\n";
    if (depth != 0) {
      llvm::errs() << "Got alias arg ptr\n";

      tableValueVar->pointers.insert(tableValuePtr);
      tableValuePtr->aliased.insert(tableValueVar);

    }
    // Split that part
    else if (tableValueVar->type == Pointer) {
      llvm::errs() << "Got alias arg ptr\n";

      for (const auto &varAliased : tableValueVar->aliased) {
        tableValuePtr->aliased.insert(varAliased);
        varAliased->pointers.insert(tableValuePtr);
      }
    }
  }
  llvm::errs() << "Done adding alias ptr\n";
}
*/
void AliasTable::removeDependencyPtr(const std::shared_ptr<aliasArg> &ptr) {
  if (ptr != nullptr) {
    for (const auto &varAliased : ptr->aliased) {
      auto tableValueVar = getAliasArg(&varAliased->declaration);
      tableValueVar->pointers.erase(ptr);
    }
    ptr->aliased.clear();
  }
}
const std::unordered_set<std::shared_ptr<aliasArg>>
AliasTable::getAliases(std::shared_ptr<aliasArg> &v) const {
  std::unordered_set<std::shared_ptr<aliasArg>> aliases, prevAliases;
  aliases.insert(v);
  int oldSize = 0;
  int newsize = aliases.size();
  while (newsize != oldSize) {
    oldSize = newsize;
    for (const auto &alias : aliases) {
      getReferencesAliases(alias, prevAliases);
      // getPointersAliases(alias,aliases);
      if (alias && alias->type == Reference)
        for (const auto &ref : alias->aliased)
          aliases.insert(ref);
    }
    for (const auto &prevAlias : prevAliases)
      aliases.insert(prevAlias);
    newsize = aliases.size();
  }
  return aliases;
};

std::shared_ptr<aliasArg>
AliasTable::getAliasArg(const VarDecl *v,
                        const std::vector<int> &indexes) const {
  // const aliasArg *result = nullptr;
  const NamedDecl *key = getKey(v);
  std::shared_ptr<aliasArg> result = nullptr;
  if (aliasTableMap.count(key) != 0) {
    auto tableValue = aliasTableMap.at(key, indexes);
    if (tableValue == nullptr)
      ;
    else if (isVariantAliasArg(*tableValue)) {
      result = getVariantAliasArg(*tableValue);
    } else if (isVariantSubArray(*tableValue)) {
      result = getVariantSubArray(*tableValue)->alias;
    } else {
      // TODO: Remove this later (if no issues arrise)
      llvm::errs() << "Error in getAliasArg\n";
      v->dump();
      return nullptr;
    }
  }
  return result;
}
std::shared_ptr<aliasArg>
AliasTable::getAliasArg(const VarDecl *v, const std::vector<int> &indexes) {
  // const aliasArg *result = nullptr;
  const NamedDecl *key = getKey(v);
  std::shared_ptr<aliasArg> result = nullptr;
  if (aliasTableMap.count(key) != 0) {
    auto tableValue = aliasTableMap.at(key, indexes);
    if (tableValue == nullptr)
      ;
    else if (isVariantAliasArg(*tableValue)) {
      result = getVariantAliasArg(*tableValue);
    } else if (isVariantSubArray(*tableValue)) {
      result = getVariantSubArray(*tableValue)->alias;
    } else {
      // TODO: Remove this later (if no issues arrise)
      llvm::errs() << "Error in getAliasArg\n";
      v->dump();
      return nullptr;
    }
  }
  return result;
}

void AliasTable::getReferencesAliases(
    std::shared_ptr<aliasArg> v,
    std::unordered_set<std::shared_ptr<aliasArg>> &aliases) const {
  if (v)
    for (const auto &alias : v->references)
      aliases.insert(alias);
}
void AliasTable::getPointersAliases(
    std::shared_ptr<aliasArg> v,
    std::unordered_set<std::shared_ptr<aliasArg>> &aliases) const {
  if (v)
    for (const auto &alias : v->pointers)
      aliases.insert(alias);
}

void AliasTable::getModifiedVariables(
    std::unordered_set<std::shared_ptr<aliasArg>> &setResults,
    const int &depth) {
  if (depth > 0) {
    int curDepth = 0;
    std::unordered_set<std::shared_ptr<aliasArg>> curSet, precSet;
    for (auto &dep : setResults)
      curSet.insert(dep);
    while (curDepth < depth) {
      precSet = curSet;
      curSet.clear();
      for (auto &dep : precSet) {
        if (dep == nullptr)
          continue;
        if (dep->type == Reference)
          ;
        else if (dep->type == Pointer) {
          if (dep->aliased.size())
            for (auto &ptr : dep->aliased)
              curSet.insert(ptr);
        }
      }
      curDepth++;
    }
    setResults.clear();
    for (auto &dep : curSet)
      setResults.insert(dep);
  }
  // Might be merged with previous case
  else if (depth == 0) {
    if (setResults.size() == 1 && (*setResults.begin())->type == Reference) {
      llvm::errs() << "RefCase\n\n";
    }
    std::unordered_set<std::shared_ptr<aliasArg>> curSet, tempAliased;
    for (auto &dep : setResults)
      curSet.insert(dep);
    // Retrieve the references to the variable
    for (auto &dep : curSet)
      tempAliased.insert(dep);
    int oldSize = 0, newSize = tempAliased.size();

    while (oldSize != newSize) {
      oldSize = newSize;
      // For each variable
      for (auto &dep : tempAliased) {
        // If its a reference, then add the aliased variables
        if (dep->type == Reference) {
          for (auto &alias : dep->aliased)
            tempAliased.insert(alias);
        }
        // We add references to the current variable to the set of modified
        // variable We don't add it to tempAliased because those references
        // can't alias a different variable Either they refer to a different
        // variable (but we couldn't be sure of it at compile time) Or they
        // refer to the current variable, in which case its other aliased
        // variables aren't really aliased
        for (auto &ref : dep->references)
          curSet.insert(ref);
      }
      newSize = tempAliased.size();
    }

    for (auto &alias : tempAliased)
      curSet.insert(alias);
    for (auto &dep : curSet)
      setResults.insert(dep);

  } else if (depth == -1)
    setResults.clear();
}

std::unordered_set<std::shared_ptr<aliasArg>>
AliasTable::getArrayElementChildren(std::shared_ptr<aliasArg> elem) const {

  std::unordered_set<std::shared_ptr<aliasArg>> children;
  if (elem != nullptr) {
    auto curElem = aliasTableMap.at(getKey(&elem->declaration), elem->indexes);
    std::stack<const aliasesTableValues *> toVisit;
    toVisit.push(curElem);
    while (!toVisit.empty()) {
      auto cur = toVisit.top();
      toVisit.pop();
      if (isVariantAliasArg(*cur)) {
        children.insert(getVariantAliasArg(*cur));
      } else if (isVariantSubArray(*cur)) {
        auto curMap = getVariantSubArray(*cur);
        for (auto &subAlias : curMap->map) {
          toVisit.push(&subAlias.second);
        }
      }
    }
  }
  return children;
}

std::vector<std::shared_ptr<aliasArg>>
AliasTable::getArrayElementParents(std::shared_ptr<aliasArg> elem) const {
  std::vector<std::shared_ptr<aliasArg>> parents;
  if (elem != nullptr) {
    auto curElem = aliasTableMap.at(getKey(&elem->declaration));
    std::stack<const aliasesTableValues *> toVisit;
    toVisit.push(curElem);
    while (!toVisit.empty()) {
      auto cur = toVisit.top();
      if (cur == nullptr)
        continue;
      toVisit.pop();
      if (isVariantAliasArg(*cur)) {
        auto alias = getVariantAliasArg(*cur);
        if (!alias->indexes.empty() &&
            (alias->indexes.size() > elem->indexes.size() ||
             alias->indexes.at(alias->indexes.size() - 1) ==
                 elem->indexes[alias->indexes.size() - 1])) {

          parents.push_back(alias);
        }
      }

      else if (isVariantSubArray(*cur)) {
        auto curMap = getVariantSubArray(*cur);
        auto alias = curMap->alias;
        if (!alias->indexes.empty() &&
            (alias->indexes.size() > elem->indexes.size() ||
             alias->indexes[alias->indexes.size() - 1] ==
                 elem->indexes[alias->indexes.size() - 1])) {

          parents.push_back(curMap->alias);
          for (auto &subAlias : curMap->map) {
            toVisit.push(&subAlias.second);
          }
        }
      }
    }
  }
  return parents;
}

std::vector<std::shared_ptr<aliasArg>>
AliasTable::getArrayElementAll(std::shared_ptr<aliasArg> elem) const {
  std::vector<std::shared_ptr<aliasArg>> related;
  if (elem != nullptr) {
    auto curElem = aliasTableMap.at(getKey(&elem->declaration));
    std::stack<const aliasesTableValues *> toVisit;
    toVisit.push(curElem);
    while (!toVisit.empty()) {
      auto cur = toVisit.top();
      toVisit.pop();
      if (isVariantAliasArg(*cur)) {
        related.push_back(getVariantAliasArg(*cur));
      } else if (isVariantSubArray(*cur)) {
        auto curMap = getVariantSubArray(*cur);
        related.push_back(curMap->alias);
        for (auto &subAlias : curMap->map) {
          toVisit.push(&subAlias.second);
        }
      }
    }
  }
  return related;
}

void AliasTable::dumpVarTable() const {
  std::string varTable;
  dumpPrep(&varTable, nullptr, nullptr);
  llvm::errs() << "Variable Table\n\n" << varTable;
  llvm::errs() << "\n";
}
void AliasTable::dumpRefTable() const {
  std::string refTable;
  dumpPrep(nullptr, &refTable, nullptr);
  llvm::errs() << "Reference Table\n\n" << refTable;
  llvm::errs() << "\n";
}
void AliasTable::dumpPtrTable() const {
  std::string ptrTable;
  dumpPrep(nullptr, nullptr, &ptrTable);
  llvm::errs() << "Pointer Table\n\n" << ptrTable;
  llvm::errs() << "\n";
}
void AliasTable::dumpPrep(std::string *varTable, std::string *refTable,
                          std::string *ptrTable) const {
  std::stringstream ssVar, ssRef, ssPtr;
  for (auto &elem : aliasTableMap.map) {
    if (isVariantAliasArg(elem.second)) {
      auto alias = getVariantAliasArg(elem.second);
      if (varTable != nullptr && alias->type == Variable)
        ssVar << alias->varAsString() << " ";
      else if (refTable != nullptr && alias->type == Reference) {
        ssRef << alias->varAsString() << " : ";
        for (auto aliased : alias->aliased)
          ssRef << aliased->varAsString() << " ";
        ssRef << "\n";
      } else if (ptrTable != nullptr && alias->type == Pointer) {
        ssPtr << alias->varAsString() << " : ";
        for (auto aliased : alias->aliased)
          ssPtr << aliased->varAsString() << " ";
        ssPtr << "\n";
      }
    } else if (isVariantSubArray(elem.second)) {
      auto alias = getVariantSubArray(elem.second);
      for (auto &subAlias : alias->map) {
        if (isVariantAliasArg(subAlias.second)) {
          auto aliasBis = getVariantAliasArg(subAlias.second);
        }
      }
      alias->dumpPrep(varTable, refTable, ptrTable);
      if (varTable != nullptr)
        ssVar << *varTable;
      if (refTable != nullptr)
        ssRef << *refTable;
      if (ptrTable != nullptr)
        ssPtr << *ptrTable;
    }
  }
  if (varTable != nullptr)
    *varTable = ssVar.str();
  if (refTable != nullptr)
    *refTable = ssRef.str();
  if (ptrTable != nullptr)
    *ptrTable = ssPtr.str();
}
