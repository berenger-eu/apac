#pragma once
#include "ElementsRepresentationStruct.hpp"
#include "common.hpp"
#include "clang/AST/Decl.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <unordered_map>
#include <unordered_set>
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
/*
struct pointersAliasArg : public aliasArg {
  pointersAliasArg(const clang::VarDecl &decl,
                   std::vector<int> indexes = std::vector<int>())
      : aliasArg(decl, Pointer, indexes) {}
  // Element referenced
  std::unordered_set<aliasArg *> aliased;
  void dump() const override;
};
struct referenceAliasArg : public aliasArg {
  referenceAliasArg(const clang::VarDecl &decl,
                    std::vector<int> indexes = std::vector<int>())
      : aliasArg(decl, Reference, indexes) {}
  // Element referenced
  std::unordered_set<aliasArg *> aliased;
  void dump() const override;
};
*/

typedef std::unordered_map<
    const NamedDecl *, std::unordered_map<std::vector<int>, struct aliasArg>>
    AliasTableMap;

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
    const aliasArg *result = nullptr;
    const NamedDecl *key = getKey(v);
    if (aliasTableMap.count(key) == 0)
      return nullptr;
    return &varAliasTable.at({result, std::vector<int>()});
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
    auto &key = getKey(v);
    if (v != nullptr)
      return;
    if (aliasTableMap.count(key) == 0)
      aliasTableMap.at(key).insert({indexes, aliasArg(*v, Variable, indexes)});
    else if (aliasTableMap.at(key).count(indexes) == 0)
      aliasTableMap.at(key).insert({indexes, aliasArg(*v, Variable, indexes)});
  }

  inline aliasArg *getAliasArg(const VarDecl *v) {
    const aliasArg *result = nullptr;
    const NamedDecl *key = getKey(v);
    if (aliasTableMap.count(key) == 0)
      return nullptr;
    return &varAliasTable.at({result, std::vector<int>()});
  }
  void getReferencesAliases(const VarDecl *,
                            std::unordered_set<const VarDecl *> &) const;
  void getPointersAliases(const VarDecl *,
                          std::unordered_set<const VarDecl *> &) const;

  AliasTableMap aliasTableMap;
  Rewriter &TheRewriter;
};
