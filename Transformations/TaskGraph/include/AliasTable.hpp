#pragma once
#include "AliasTableMaps.hpp"
#include "ElementsRepresentationStruct.hpp"
#include "common.hpp"
#include "clang/AST/Decl.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
using namespace clang;

class AliasTable {
public:
  AliasTable(Rewriter &R) : TheRewriter(R) {}

  void inline dump() const {
    std::string varTable, refTable, ptrTable;
    llvm::errs() << "Map Size : " << aliasTableMap.map.size()
                 << "\nDumping Alias Table\n";
    dumpPrep(&varTable, &refTable, &ptrTable);
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
  void addAliasPtr(const Expr *var, const Expr *ptr);
  void addAliasPtr(const VarDecl *var, const std::vector<int> &,
                   const VarDecl *ptr, const std::vector<int> &, const int &);
  void getModifiedVariables(std::unordered_set<const VarDecl *> &setResults,
                            const int &depth);

  std::shared_ptr<const aliasArg>
  getAliasArg(const VarDecl *v,
              const std::vector<int> & = std::vector<int>()) const;

private:
  void dumpPrep(std::string *varTable, std::string *refTable,
                std::string *ptrTable) const;
  inline const NamedDecl *getKey(const VarDecl *v) const {
    return v->getCanonicalDecl();
  }
  void addElementToAliasTable(const VarDecl *v, const AliasType &type,
                              std::vector<int> indexes = std::vector<int>()) {
    // const auto &key = getKey(v);
    if (v != nullptr) {
      llvm::errs() << "Adding element to alias table\n";
      aliasTableMap.insert({aliasArg(*v, type, indexes), indexes});
    }
  }

  std::shared_ptr<aliasArg>
  getAliasArg(const VarDecl *v,
              const std::vector<int> &indexes = std::vector<int>());
  void getReferencesAliases(const VarDecl *,
                            std::unordered_set<const VarDecl *> &) const;
  void getPointersAliases(const VarDecl *,
                          std::unordered_set<const VarDecl *> &) const;

  AliasTableMapStruct aliasTableMap;
  Rewriter &TheRewriter;
};
