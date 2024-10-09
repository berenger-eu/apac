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

struct IndexTableMapStruct;
using aliasesTableValues = std::variant<std::shared_ptr<aliasArg>,
                                        std::shared_ptr<IndexTableMapStruct>>;
using IndexTableMap = std::map<int, aliasesTableValues>;
// First Key is the variable, the next key(s) will be the indexes
struct IndexTableMapStruct {
  IndexTableMap map;
  // current alias, for if map corresponds to tab[1][x] then alias is tab[1]
  std::shared_ptr<aliasArg> alias;

  aliasesTableValues *at(const std::vector<int> &indexes);
  const aliasesTableValues *at(const std::vector<int> &indexes) const;

  int count(const std::vector<int> &indexes) const;
  int nbElements() const;
  void insert(const std::pair<aliasArg, std::vector<int> &> pair);

  void dumpPrep(std::string *varTable, std::string *refTable,
                std::string *ptrTable) const;
};
struct AliasTableMapStruct;
using AliasTableMap = std::unordered_map<const NamedDecl *, aliasesTableValues>;

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
inline bool isSubArray(const aliasesTableValues &value) {
  return std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(value);
}
inline std::shared_ptr<IndexTableMapStruct>
getSubArray(const aliasesTableValues &value) {
  assert(isSubArray(value));
  return std::get<std::shared_ptr<IndexTableMapStruct>>(value);
}
inline bool isAliasArg(const aliasesTableValues &value) {
  return std::holds_alternative<std::shared_ptr<aliasArg>>(value);
}
inline std::shared_ptr<aliasArg> getAliasArg(const aliasesTableValues &value) {
  assert(isAliasArg(value));
  return std::get<std::shared_ptr<aliasArg>>(value);
}