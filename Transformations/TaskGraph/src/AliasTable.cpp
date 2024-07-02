#include "AliasTable.hpp"

using namespace clang;

void aliasArg::dump() const {
  llvm::errs() << "AliasArg: " << declaration.getNameAsString() << "\n";
  switch (type) {
  case Reference:
    llvm::errs() << "Type: Reference\n";
    break;
  case Pointer:
    llvm::errs() << "Type: Pointer\n";
    break;
  case Variable:
    llvm::errs() << "Type: Variable\n";
    break;
  default:
    llvm::errs() << "Type: Unknown\n";
    break;
  }
  llvm::errs() << "Pointers: ";
  for (const auto &ptr : pointers)
    llvm::errs() << ptr->declaration.getNameAsString() << " ";
  llvm::errs() << "\n";
  llvm::errs() << "References: ";
  for (const auto &ref : references)
    llvm::errs() << ref->declaration.getNameAsString() << " ";
  llvm::errs() << "\n";
}
void pointersAliasArg::dump() const {
  aliasArg::dump();
  llvm::errs() << "Aliased: ";
  for (const auto &aliased : aliased)
    llvm::errs() << aliased->declaration.getNameAsString() << " ";
  llvm::errs() << "\n";
}
void referenceAliasArg::dump() const {
  aliasArg::dump();
  llvm::errs() << "Aliased: ";
  for (const auto &aliased : aliased)
    llvm::errs() << aliased->declaration.getNameAsString() << " ";
  llvm::errs() << "\n";
}
std::unordered_set<const VarDecl *> AliasTable::getAliased(const VarDecl *v) {
  std::unordered_set<const VarDecl *> aliases;
  std::stack<aliasArg *> stack;
  auto alias = getAliasArg(v);
  if (alias != nullptr)
    stack.push(alias);
  while (!stack.empty()) {
    const aliasArg *cur = stack.top();
    stack.pop();
    if (aliases.count(&cur->declaration) != 0)
      continue;
    aliases.insert(&cur->declaration);
    if (cur->type == Reference) {
      const referenceAliasArg *ref =
          static_cast<const referenceAliasArg *>(cur);
      for (const auto &alias : ref->aliased)
        stack.push(alias);
    } else if (cur->type == Pointer) {
      const pointersAliasArg *ptr = static_cast<const pointersAliasArg *>(cur);
      for (const auto &alias : ptr->aliased)
        stack.push(alias);
    }
  }

  return aliases;
}
void AliasTable::addAliasReference(const VarDecl *var, const VarDecl *ref) {
  if (var != nullptr && ref != nullptr) {
    referenceAliasArg *tableValueRef = getRefAliasArg(ref);
    aliasArg *tableValueVar = getAliasArg(var);
    if (tableValueRef == nullptr) {
      refAliasTable.insert(
          {{getKey(ref), std::vector<int>()}, referenceAliasArg{*ref}});
      tableValueRef = getRefAliasArg(ref);
    }
    if (tableValueVar == nullptr) {
      varAliasTable.insert({{getKey(var), std::vector<int>()},
                            aliasArg{*var, AliasType::Variable}});
      tableValueVar = getAliasArg(var);
    }
    tableValueVar->references.insert(tableValueRef);
    tableValueRef->aliased.insert(tableValueVar);
  }
}
void AliasTable::addAliasPtr(const VarDecl *var, const VarDecl *ptr) {
  if (var != nullptr && ptr != nullptr) {
    pointersAliasArg *tableValuePtr = getPtrAliasArg(ptr);
    aliasArg *tableValueVar = getAliasArg(var);
    if (tableValuePtr == nullptr) {
      ptrAliasTable.insert(
          {{getKey(ptr), std::vector<int>()}, pointersAliasArg{*ptr}});
      tableValuePtr = getPtrAliasArg(ptr);
    }
    if (isPointerQualType(var->getType())) {
      if (tableValueVar == nullptr) {
        ptrAliasTable.insert(
            {{getKey(var), std::vector<int>()}, pointersAliasArg{*var}});
        tableValueVar = getAliasArg(var);
      }
    } else {
      if (tableValueVar == nullptr) {
        varAliasTable.insert({{getKey(var), std::vector<int>()},
                              aliasArg{*var, AliasType::Variable}});
        tableValueVar = getAliasArg(var);
      }
    }
    if (getPtrDepthAccess(var->getType(), ptr->getType(),
                          var->getASTContext()) != 0) {
      tableValueVar->pointers.insert(tableValuePtr);
      tableValuePtr->aliased.insert(tableValueVar);
    } else if (tableValueVar->type == Pointer) {
      pointersAliasArg *tableValuePtr2 =
          static_cast<pointersAliasArg *>(tableValueVar);
      for (const auto &varAliased : tableValuePtr2->aliased) {
        tableValuePtr->aliased.insert(varAliased);
        varAliased->pointers.insert(tableValuePtr);
      }
    }
  }
}
void AliasTable::removeDependencyPtr(const VarDecl *ptr) {
  if (ptr != nullptr) {
    pointersAliasArg *tableValuePtr = getPtrAliasArg(ptr);
    if (tableValuePtr) {
      for (const auto &varAliased : tableValuePtr->aliased) {
        aliasArg *tableValueVar = getAliasArg(&varAliased->declaration);
        tableValueVar->pointers.erase(tableValuePtr);
      }
      tableValuePtr->aliased.clear();
    }
  }
}
const std::unordered_set<const VarDecl *>
AliasTable::getAliases(const VarDecl *v) const {
  std::unordered_set<const VarDecl *> aliases, prevAliases;
  aliases.insert(v);
  int oldSize = 1;
  int newsize = 1;
  do {
    oldSize = newsize;
    for (const auto &alias : aliases) {
      getReferencesAliases(alias, prevAliases);
      // getPointersAliases(alias,aliases);
      const referenceAliasArg *refAlias = getRefAliasArg(alias);
      if (refAlias)
        for (const auto &ref : refAlias->aliased)
          aliases.insert(&ref->declaration);
    }
    for (const auto &prevAlias : prevAliases)
      aliases.insert(prevAlias);
    newsize = aliases.size();
  } while (newsize != oldSize);
  return aliases;
};
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
    std::unordered_set<const aliasArg *> curSet, precSet;
    for (auto &dep : setResults)
      curSet.insert(getAliasArg(dep));
    while (curDepth < depth) {
      precSet = curSet;
      curSet.clear();
      for (auto &dep : precSet) {
        if (dep == nullptr)
          continue;
        if (dep->type == Reference)
          const referenceAliasArg *ref =
              static_cast<const referenceAliasArg *>(dep);
        else if (dep->type == Pointer) {
          const pointersAliasArg *dep2 =
              static_cast<const pointersAliasArg *>(dep);
          if (dep2->aliased.size())
            for (auto &ptr : dep2->aliased)
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
    std::unordered_set<const aliasArg *> curSet, tempAliased;
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
          const referenceAliasArg *ref =
              static_cast<const referenceAliasArg *>(dep);
          for (auto &alias : ref->aliased)
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
  llvm::errs() << "Variable Table\n\n";
  for (auto &var : varAliasTable) {
    var.first.dump();
    var.second.dump();
  }
  llvm::errs() << "\n";
}
void AliasTable::dumpRefTable() const {
  llvm::errs() << "Reference Table\n\n";
  for (auto &ref : refAliasTable) {
    ref.first.dump();
    ref.second.dump();
  }
  llvm::errs() << "\n";
}
void AliasTable::dumpPtrTable() const {
  llvm::errs() << "Pointer Table\n\n";
  for (auto &ptr : ptrAliasTable) {
    ptr.first.dump();
    ptr.second.dump();
  }
  llvm::errs() << "\n";
}
