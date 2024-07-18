#pragma once
#include "common.hpp"
#include "clang/AST/Decl.h"
#include <memory>
#include <unordered_set>
#include <vector>
using namespace clang;
enum AliasType { Reference, Pointer, Variable };
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
  std::string varAsString() const {
    std::stringstream ssRes;
    ssRes << declaration.getNameAsString();
    for (const auto &index : indexes)
      ssRes << "[" << index << "]";
    return ssRes.str();
  }
  std::string dumpAsStr() const {
    std::stringstream ssRes;
    ssRes << "AliasArg: " << varAsString() << "\n";
    switch (type) {
    case Reference:
      ssRes << "Type: Reference\n";
      break;
    case Pointer:
      ssRes << "Type: Pointer\n";
      break;
    case Variable:
      ssRes << "Type: Variable\n";
      break;
    default:
      ssRes << "Type: Unknown\n";
      break;
    }
    ssRes << "Pointers: ";
    for (const auto &ptr : pointers)
      ssRes << ptr->declaration.getNameAsString() << " ";
    ssRes << "\n";
    ssRes << "References: ";
    for (const auto &ref : references)
      ssRes << ref->declaration.getNameAsString() << " ";
    ssRes << "\n";
    if (type == Reference || type == Pointer) {
      ssRes << "Aliased: ";
      for (const auto &aliased : aliased)
        ssRes << aliased->declaration.getNameAsString() << " ";
      ssRes << "\n";
    }
    return ssRes.str();
  }
  inline void dump() const { llvm::errs() << dumpAsStr(); }
};