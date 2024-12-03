#pragma once
#include "common.hpp"
#include "clang/AST/Decl.h"
#include <memory>
#include <unordered_set>
#include <vector>
using namespace clang;
enum AliasType { Reference, Pointer, Variable, None };
inline AliasType getAliasType(const Expr *exp) {
  const auto &qType = exp->getType();
  if (isPointerQualType(qType))
    return Pointer;
  else if (isReferenceQualType(qType))
    return Reference;
  return Variable;
}
inline AliasType getAliasType(const ValueDecl *v) {
  const auto &qType = v->getType();
  if (isPointerQualType(qType))
    return Pointer;
  else if (isReferenceQualType(qType))
    return Reference;
  return Variable;
}
struct aliasArg {
  bool hasUnknownIndex = false;
  // To store the indexes of the array (mostly for unknown indexes such as
  // "a+1",etc)
  const clang::VarDecl &declaration;
  // Type of Alias arg
  const AliasType type;
  const std::vector<int> indexes;
  std::string indexString;
  int id;
  static int curId;
  // Elements that point to current element
  std::unordered_set<std::shared_ptr<aliasArg>> pointers;
  // Elements that references current element
  std::unordered_set<std::shared_ptr<aliasArg>> references;
  // Elements aliased(mostly only for pointers and references)
  std::unordered_set<std::shared_ptr<aliasArg>> aliased;

  aliasArg(const clang::VarDecl &decl, AliasType t,
           std::vector<int> indexes = std::vector<int>(),
           std::string indexString = "")
      : declaration(decl), type(t), indexes(indexes), indexString(indexString) {
    id = curId++;
    for (int &index : indexes)
      if (index == -1) {
        assert(!indexString.empty());
        hasUnknownIndex = true;
      }
  }
  std::string varAsString() const;
  std::string dumpAsStr() const;
  inline void dump() const { llvm::errs() << dumpAsStr(); }
  // Get the key of the aliasArg (used for the map when there are unknown
  // indexes)
  // Currently, it's indexString, example: "[a][0][4]" (for tab[a][0][4])
  inline std::string getIndexKey() const {
    return hasUnknownIndex ? indexString : "";
  }
};
// To compare two indexes while considering unknown indexes (-1), so that a[1]
// and a[d] will match
inline bool indexesMatch(const std::vector<int> &indexes1,
                         const std::vector<int> &indexes2) {
  if (indexes1.size() != indexes2.size())
    return false;
  for (size_t i = 0; i < indexes1.size(); i++)
    if (indexes1[i] != indexes2[i] && indexes1[i] != -1 && indexes2[i] != -1)
      return false;
  return true;
}