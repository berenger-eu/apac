#pragma once
#include "ElementsRepresentationStruct.hpp"
#include "common.hpp"
#include "clang/AST/Decl.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
using namespace clang;
enum AliasType { Reference, Pointer, Variable };
struct pointersAliasArg;
struct referenceAliasArg;
struct aliasArg {
  const clang::VarDecl &declaration;
  // Type of Alias arg
  const AliasType type;
  const std::vector<int> indexes;
  // Elements that point to current element
  std::unordered_set<pointersAliasArg *> pointers;
  // Elements that references current element
  std::unordered_set<referenceAliasArg *> references;
  // Elements aliased(mostly only for pointers and references)
  std::unordered_set<aliasArg *> aliased;

  aliasArg(const clang::VarDecl &decl, AliasType t,
           std::vector<int> indexes = std::vector<int>())
      : declaration(decl), type(t), indexes(indexes) {}

  virtual void dump() const;
};
struct IndexTableMapStruct;
using aliasesTableValues =
    std::variant<aliasArg *, std::shared_ptr<IndexTableMapStruct>>;
using IndexTableMap = std::map<int, aliasesTableValues>;
// First Key is the variable, the next key(s) will be the indexes
struct IndexTableMapStruct {
  IndexTableMap map;
  // current alias, for if map corresponds to tab[1][x] then alias is tab[1]
  aliasArg *alias;
  aliasesTableValues *at(const std::vector<int> &indexes) {
    // If no indexes, return nullptr
    if (indexes.empty())
      return nullptr;
    // If only one index, return the value
    if (indexes.size() == 1 && map.count(indexes[0]))
      return &map.at(indexes[0]);
    // If the value is an IndexTableMapStruct, we look through it using indexes
    else if (map.count(indexes[0])) {
      if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
              map.at(indexes[0])))
        return std::get<std::shared_ptr<IndexTableMapStruct>>(
                   map.at(indexes[0]))
            ->at(std::vector<int>(indexes.begin() + 1, indexes.end()));
    }
    // Otherwise, the value is simple variable and we're trying to access an
    // index, so we return nullptr
    else
      return nullptr;
  }
  const aliasesTableValues *at(const std::vector<int> &indexes) const {
    // If no indexes, return nullptr
    if (indexes.empty())
      return nullptr;
    // If only one index, return the value
    if (indexes.size() == 1 && map.count(indexes[0]))
      return &map.at(indexes[0]);
    // If the value is an IndexTableMapStruct, we look through it using indexes
    else if (map.count(indexes[0])) {
      if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
              map.at(indexes[0])))
        return std::get<std::shared_ptr<IndexTableMapStruct>>(
                   map.at(indexes[0]))
            ->at(std::vector<int>(indexes.begin() + 1, indexes.end()));
    }
    // Otherwise, the value is simple variable and we're trying to access an
    // index, so we return nullptr
    else
      return nullptr;
  }
  int count(const std::vector<int> &indexes) const {
    if (indexes.empty())
      return 0;
    if (indexes.size() == 1)
      return (map.count(indexes[0]) > 0);
    if (map.count(indexes[0]) == 0)
      return 0;
    return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(indexes[0]))
        ->count(std::vector<int>(indexes.begin() + 1, indexes.end()));
  }
};
struct AliasTableMapStruct;
using AliasTableMap = std::unordered_map<const NamedDecl *, aliasesTableValues>;
struct AliasTableMapStruct {
  AliasTableMap map;
  aliasesTableValues *at(const NamedDecl *key,
                         const std::vector<int> &indexes = std::vector<int>()) {
    // If no entry for variable, return nullptr
    if (map.count(key) == 0)
      return nullptr;
    // If no indexes, return the value
    if (indexes.empty()) {
      map.at(key);
    }
    // If the value is an aliasArg (and not an array, so no indexes), return
    // nullptr
    else if (std::holds_alternative<aliasArg *>(map.at(key)))
      return nullptr;
    // The value is an array, so we look through it using indexes
    else
      return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key))
          ->at(indexes);
  }
  const aliasesTableValues *
  at(const NamedDecl *key,
     const std::vector<int> &indexes = std::vector<int>()) const {
    // If no entry for variable, return nullptr
    if (map.count(key) == 0)
      return nullptr;
    // If no indexes, return the value
    if (indexes.empty()) {
      map.at(key);
    }
    // If the value is an aliasArg (and not an array, so no indexes), return
    // nullptr
    else if (std::holds_alternative<aliasArg *>(map.at(key)))
      return nullptr;
    // The value is an array, so we look through it using indexes
    else
      return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key))
          ->at(indexes);
  }
  int count(const NamedDecl *key,
            const std::vector<int> &indexes = std::vector<int>()) const {
    if (indexes.empty())
      return map.count(key);
    else if (map.count(key) != 0) {
      if (std::holds_alternative<aliasArg *>(map.at(key)))
        return 0;
      return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key))
          ->count(indexes);
    }
  }
};
class AliasTable {
public:
  AliasTable(Rewriter &R) : TheRewriter(R) {}

  void inline dump() const {
    std::string varTable, refTable, ptrTable;
    dumpPrep(varTable, refTable, ptrTable);
    llvm::errs() << "Var Table \n\n"
                 << varTable << "\nRef Table\n\n"
                 << refTable << "\nPtr Table \n\n"
                 << ptrTable << "\n";
  };
  void dumpPtrTable() const;
  void dumpRefTable() const;
  void dumpVarTable() const;

  const std::unordered_set<const VarDecl *> getAliases(const VarDecl *v) const;
  std::unordered_set<const VarDecl *> getAliased(const VarDecl *v);
  void removeDependencyPtr(const VarDecl *ptr);
  void addAliasReference(const VarDecl *var, const VarDecl *ref);
  void addAliasPtr(const VarDecl *var, const VarDecl *ptr);
  void getModifiedVariables(std::unordered_set<const VarDecl *> &setResults,
                            const int &depth);

  inline const aliasArg *getAliasArg(const VarDecl *v) const {
    const NamedDecl *key = getKey(v);
    if (aliasTableMap.count(key) == 0)
      return nullptr;
    if (std::holds_alternative<aliasArg *>(*aliasTableMap.at(key)))
      return std::get<aliasArg *>(*aliasTableMap.at(key));
    else {
      // TODO: Remove this later (if no issues arrise)
      llvm::errs() << "Error in getAliasArgConst\n";
      v->dump();
      return nullptr;
    }
  }

private:
  void dumpPrep(std::string varTable, std::string refTable,
                std::string ptrTable) const;
  inline const NamedDecl *getKey(const VarDecl *v) const {
    return v->getCanonicalDecl();
  }
  void
  addElementToVarAliasTable(const VarDecl *v,
                            std::vector<int> indexes = std::vector<int>()) {
    const auto &key = getKey(v);
    if (v != nullptr)
      return;
    if (aliasTableMap.count(key) == 0)
      aliasTableMap.at(key)->insert({indexes, aliasArg(*v, Variable, indexes)});
    else if (aliasTableMap.count(key, indexes) == 0)
      aliasTableMap.at(key)->insert({indexes, aliasArg(*v, Variable, indexes)});
  }

  inline aliasArg *getAliasArg(const VarDecl *v) {
    const aliasArg *result = nullptr;
    const NamedDecl *key = getKey(v);
    if (aliasTableMap.count(key) == 0)
      return nullptr;
    if (std::holds_alternative<aliasArg *>(*aliasTableMap.at(key)))
      return std::get<aliasArg *>(*aliasTableMap.at(key));
    else {
      // TODO: Remove this later (if no issues arrise)
      llvm::errs() << "Error in getAliasArg\n";
      v->dump();
      return nullptr;
    }
  }
  void getReferencesAliases(const VarDecl *,
                            std::unordered_set<const VarDecl *> &) const;
  void getPointersAliases(const VarDecl *,
                          std::unordered_set<const VarDecl *> &) const;

  AliasTableMapStruct aliasTableMap;
  Rewriter &TheRewriter;
};
