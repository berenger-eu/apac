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
  std::unordered_set<std::shared_ptr<aliasArg>> pointers;
  // Elements that references current element
  std::unordered_set<std::shared_ptr<aliasArg>> references;
  // Elements aliased(mostly only for pointers and references)
  std::unordered_set<std::shared_ptr<aliasArg>> aliased;

  aliasArg(const clang::VarDecl &decl, AliasType t,
           std::vector<int> indexes = std::vector<int>())
      : declaration(decl), type(t), indexes(indexes) {}

  void dump() const;
};
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
  int count(const NamedDecl *key,
            const std::vector<int> &indexes = std::vector<int>()) const;
  void insert(const std::pair<aliasArg, std::vector<int> &> pair);
};