#include "AliasTable.hpp"

using namespace clang;

std::unordered_set<const VarDecl *> AliasTable::getAliased(const VarDecl *v) {
  std::unordered_set<const VarDecl *> aliases;
  std::stack<std::shared_ptr<aliasArg>> stack;
  auto alias = getAliasArg(v);
  if (alias != nullptr)
    stack.push(alias);
  while (!stack.empty()) {
    const auto cur = stack.top();
    stack.pop();
    if (aliases.count(&cur->declaration) != 0)
      continue;
    aliases.insert(&cur->declaration);
    if (cur->type == Reference || cur->type == Pointer)
      for (const auto &alias : cur->aliased)
        stack.push(alias);
  }

  return aliases;
}
void AliasTable::addAliasReference(const VarDecl *var, const VarDecl *ref) {}
void AliasTable::addAliasPtr(const VarDecl *var, const VarDecl *ptr) {}
void AliasTable::removeDependencyPtr(const VarDecl *ptr) {
  if (ptr != nullptr) {
    auto tableValuePtr = getAliasArg(ptr);
    if (tableValuePtr) {
      for (const auto &varAliased : tableValuePtr->aliased) {
        auto tableValueVar = getAliasArg(&varAliased->declaration);
        tableValueVar->pointers.erase(tableValuePtr);
      }
      tableValuePtr->aliased.clear();
    }
  }
}
const std::unordered_set<const VarDecl *>
AliasTable::getAliases(const VarDecl *v) const {
  std::unordered_set<const VarDecl *> aliases, prevAliases;

  return aliases;
};

std::shared_ptr<const aliasArg>
AliasTable::getAliasArg(const VarDecl *v) const {
  // const aliasArg *result = nullptr;
  const NamedDecl *key = getKey(v);
  if (aliasTableMap.count(key) == 0)
    return nullptr;
  if (std::holds_alternative<std::shared_ptr<aliasArg>>(*aliasTableMap.at(key)))
    return std::get<std::shared_ptr<aliasArg>>(*aliasTableMap.at(key));
  else {
    // TODO: Remove this later (if no issues arrise)
    llvm::errs() << "Error in getAliasArg\n";
    v->dump();
    return nullptr;
  }
}
std::shared_ptr<aliasArg> AliasTable::getAliasArg(const VarDecl *v) {
  const NamedDecl *key = getKey(v);
  if (aliasTableMap.count(key) == 0)
    return nullptr;
  if (std::holds_alternative<std::shared_ptr<aliasArg>>(*aliasTableMap.at(key)))
    return std::get<std::shared_ptr<aliasArg>>(*aliasTableMap.at(key));
  else {
    // TODO: Remove this later (if no issues arrise)
    llvm::errs() << "Error in getAliasArgConst\n";
    v->dump();
    return nullptr;
  }
}

void AliasTable::getReferencesAliases(
    const VarDecl *v, std::unordered_set<const VarDecl *> &aliases) const {
  auto refAlias = getAliasArg(v);
  if (refAlias)
    for (const auto &alias : refAlias->references)
      aliases.insert(&alias->declaration);
}
void AliasTable::getPointersAliases(
    const VarDecl *v, std::unordered_set<const VarDecl *> &aliases) const {
  auto ptrAlias = getAliasArg(v);
  if (ptrAlias)
    for (const auto &alias : ptrAlias->pointers)
      aliases.insert(&alias->declaration);
}

void AliasTable::getModifiedVariables(
    std::unordered_set<const VarDecl *> &setResults, const int &depth) {
  if (depth > 0) {
    int curDepth = 0;
    std::unordered_set<std::shared_ptr<aliasArg>> curSet, precSet;
    for (auto &dep : setResults)
      curSet.insert(getAliasArg(dep));
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
      setResults.insert(&dep->declaration);
  }
  // Might be merged with previous case
  else if (depth == 0) {
    if (isReferenceQualType((*setResults.begin())->getType())) {
      llvm::errs() << "RefCase\n\n";
      dumpRefTable();
      (*setResults.begin())->dump();
    }
    std::unordered_set<std::shared_ptr<const aliasArg>> curSet, tempAliased;
    for (auto &dep : setResults)
      if (getAliasArg(dep) != nullptr)
        curSet.insert(getAliasArg(dep));
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
      setResults.insert(&dep->declaration);
    for (auto &dep : setResults)
      dep->dump();
    llvm::errs() << "done\n";
  } else if (depth == -1)
    setResults.clear();
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
    if (std::holds_alternative<std::shared_ptr<aliasArg>>(elem.second)) {
      auto alias = std::get<std::shared_ptr<aliasArg>>(elem.second);
      if (varTable != nullptr && alias->type == Variable)
        ssVar << alias->declaration.getNameAsString() << " ";
      else if (refTable != nullptr && alias->type == Reference)
        ssRef << alias->declaration.getNameAsString() << " ";
      else if (ptrTable != nullptr && alias->type == Pointer)
        ssPtr << alias->declaration.getNameAsString() << " ";
    } else if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
                   elem.second)) {
      auto alias = std::get<std::shared_ptr<IndexTableMapStruct>>(elem.second);
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
