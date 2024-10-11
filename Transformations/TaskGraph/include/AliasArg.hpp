#pragma once
#include "common.hpp"
#include "clang/AST/Decl.h"
#include <memory>
#include <unordered_set>
#include <vector>
using namespace clang;
enum AliasType { Reference, Pointer, Variable };
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
  std::string varAsString() const;
  std::string dumpAsStr() const;
  inline void dump() const { llvm::errs() << dumpAsStr(); }
};