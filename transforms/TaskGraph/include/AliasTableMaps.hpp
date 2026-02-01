#pragma once
#include "AliasArg.hpp"
#include "ElementsRepresentationStruct.hpp"
#include "common.hpp"
#include "clang/AST/Decl.h"

#include "clang/Rewrite/Core/Rewriter.h"
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

using namespace clang;

struct aliasArgStruct {
  // Main aliasArg
  std::shared_ptr<aliasArg> alias;
  // To associate unknown indexes with their own aliasArg
  std::unordered_map<std::string, std::shared_ptr<aliasArg>>
      unknownIndexAliases;
  inline int nbElements() const {
    int res = 0;
    if (alias != nullptr)
      res++;
    res += unknownIndexAliases.size();
    return res;
  }
  inline std::shared_ptr<aliasArg> getAlias() { return alias; }
  inline std::vector<std::shared_ptr<aliasArg>> getAllAlias() const {
    std::vector<std::shared_ptr<aliasArg>> aliases;
    if (alias != nullptr)
      aliases.push_back(alias);
    for (auto &alias : unknownIndexAliases)
      aliases.push_back(alias.second);
    return aliases;
  }
  inline void operator+=(const aliasArgStruct &rhs) {
    for (const auto &pair : rhs.unknownIndexAliases)
      this->unknownIndexAliases.insert(pair);
  }
};

struct IndexTableMapStruct;
struct AliasTableMapStruct;

using aliasesTableValues = std::variant<std::shared_ptr<aliasArgStruct>,
                                        std::shared_ptr<IndexTableMapStruct>>;
using IndexTableMap = std::map<int, aliasesTableValues>;

using AliasTableMap = std::unordered_map<const NamedDecl *, aliasesTableValues>;

// First Key is the variable, the next key(s) will be the indexes
struct IndexTableMapStruct {
  IndexTableMap map;
  // current alias, for if map corresponds to tab[1][x] then alias is tab[1]
  std::shared_ptr<aliasArgStruct> aliasStruct;

  aliasesTableValues *at(const std::vector<int> &indexes);
  const aliasesTableValues *at(const std::vector<int> &indexes) const;

  int count(const std::vector<int> &indexes) const;
  int nbElements() const;
  void insert(const std::pair<aliasArg, std::vector<int> &> pair);

  void dumpPrep(std::string *varTable, std::string *refTable,
                std::string *ptrTable) const;
  inline void dumpPrepAlias(std::shared_ptr<aliasArg> alias,
                            std::stringstream &ssVar, std::stringstream &ssRef,
                            std::stringstream &ssPtr) const {
    if (alias == nullptr)
      return;
    auto type = alias->type;
    if (type == Variable)
      ssVar << alias->varAsString() << " ";
    else if (type == Reference)
      ssRef << alias->varAsString() << " ";
    else if (type == Pointer)
      ssPtr << alias->varAsString() << " ";
  }
  inline void dumpPrepAliasStruct(std::shared_ptr<aliasArgStruct> aliasStruct,
                                  std::stringstream &ssVar,
                                  std::stringstream &ssRef,
                                  std::stringstream &ssPtr) const {
    if (aliasStruct == nullptr)
      return;
    dumpPrepAlias(aliasStruct->alias, ssVar, ssRef, ssPtr);
    for (auto &aliasUnknownIndexValues : aliasStruct->unknownIndexAliases) {
      dumpPrepAlias(aliasUnknownIndexValues.second, ssVar, ssRef, ssPtr);
    }
  }
};

struct AliasTableMapStruct {
  AliasTableMap map;

  aliasesTableValues *at(const NamedDecl *key,
                         const std::vector<int> &indexes = std::vector<int>());
  const aliasesTableValues *
  at(const NamedDecl *key,
     const std::vector<int> &indexes = std::vector<int>()) const;

  int nbElements() const;
  int count(const NamedDecl *key,
            const std::vector<int> &indexes = std::vector<int>()) const;
  void insert(const std::pair<aliasArg, std::vector<int> &> pair);
};

inline bool isVariantSubArray(const aliasesTableValues &value) {
  return std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(value);
}
inline std::shared_ptr<IndexTableMapStruct>
getVariantSubArray(const aliasesTableValues &value) {
  assert(isVariantSubArray(value));
  return std::get<std::shared_ptr<IndexTableMapStruct>>(value);
}
inline bool isVariantAliasArgStruct(const aliasesTableValues &value) {
  return std::holds_alternative<std::shared_ptr<aliasArgStruct>>(value);
}
inline std::shared_ptr<aliasArgStruct>
getVariantAliasArgStruct(const aliasesTableValues &value) {
  assert(isVariantAliasArgStruct(value));
  return std::get<std::shared_ptr<aliasArgStruct>>(value);
}