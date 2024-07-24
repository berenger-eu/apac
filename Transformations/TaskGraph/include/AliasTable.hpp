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
    llvm::errs() << "Map Size : " << getNbElements()
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

  const std::unordered_set<std::shared_ptr<aliasArg>>
  getAliases(std::shared_ptr<aliasArg> &v) const;
  std::unordered_set<std::shared_ptr<aliasArg>>
  getAliased(std::shared_ptr<aliasArg> &v);
  void removeDependencyPtr(const std::shared_ptr<aliasArg> &ptr);
  void addAliasReference(std::shared_ptr<aliasArg> &var,
                         std::shared_ptr<aliasArg> &ref);
  // TODO: split function ?
  void addAliasPtr(std::shared_ptr<aliasArg> var,
                   std::shared_ptr<aliasArg> ptr);
  /*
void addAliasPtr(const Expr *var, const Expr *ptr);
void addAliasPtr(const VarDecl *var, const std::vector<int> &,
  const VarDecl *ptr, const std::vector<int> &, const int &);
  */
  // Used to add aliased element of right AliasArg to left AliasArg
  void addAliasedToElement(std::shared_ptr<aliasArg>,
                           std::shared_ptr<aliasArg>);
  void getModifiedVariables(
      std::unordered_set<std::shared_ptr<aliasArg>> &setResults,
      const int &depth);

  std::shared_ptr<aliasArg>
  getAliasArg(const VarDecl *v,
              const std::vector<int> & = std::vector<int>()) const;
  std::shared_ptr<aliasArg>
  getAliasArg(const VarDecl *v,
              const std::vector<int> &indexes = std::vector<int>());
  inline bool
  isInTable(const VarDecl *v,
            const std::vector<int> &indexes = std::vector<int>()) const {
    return aliasTableMap.count(getKey(v), indexes) > 0;
  }
  inline std::shared_ptr<aliasArg>
  getOrAddAliasArg(const VarDecl *v, const AliasType &type,
                   const std::vector<int> &indexes = std::vector<int>()) {
    // If variable does not exist in table add it
    if (getAliasArg(v) == nullptr)
      addElementToAliasTable(v, type);
    // If element (variable and indexes) does not exist in table add it
    if (getAliasArg(v, indexes) == nullptr)
      addElementToAliasTable(v, type, indexes);

    return getAliasArg(v, indexes);
  }
  const AliasTableMapStruct &getAliasTable() const { return aliasTableMap; }
  int getNbElements() const { return aliasTableMap.nbElements(); }

private:
  void dumpPrep(std::string *varTable, std::string *refTable,
                std::string *ptrTable) const;
  inline const NamedDecl *getKey(const VarDecl *v) const {
    return v->getCanonicalDecl();
  }
  void addElementToAliasTable(const VarDecl *v, const AliasType &type,
                              std::vector<int> indexes = std::vector<int>()) {
    const auto &key = getKey(v);
    if (key != nullptr) {
      llvm::errs() << "Adding element to alias table\n";
      aliasTableMap.insert({aliasArg(*v, type, indexes), indexes});
    }
  }

  void
  getReferencesAliases(std::shared_ptr<aliasArg>,
                       std::unordered_set<std::shared_ptr<aliasArg>> &) const;
  void
  getPointersAliases(std::shared_ptr<aliasArg>,
                     std::unordered_set<std::shared_ptr<aliasArg>> &) const;

  AliasTableMapStruct aliasTableMap;
  Rewriter &TheRewriter;
};
