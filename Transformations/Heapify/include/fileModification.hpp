#pragma once
#include "core.hpp"
#include "utilitaryFunctions.hpp"
using namespace clang;

void modifyFile(const std::unordered_map<CompoundStmt *,
                                         std::shared_ptr<ScopeInfo>> &scopes,
                Rewriter &TheRewriter);
// Creates a string for the deletion of a variable (delete <varName>)
std::string createDeleteString(VarDecl *var);
// Creates a segment containing all the delete strings
std::string createDeleteSegment(const std::vector<VarDecl *> &varsToDelete);
std::string createCreationStringNonArray(const struct item_found &itFound,
                                         const LangOptions &langOpts);
std::string createCreationStringArray(const struct item_found &itFound,
                                      const LangOptions &langOpts);
void initItem(struct item_found &item, VarDecl &vDec);
// Builds and returns the string : "apacMemeBloc__varName_varId"

inline std::string getApacMemBlockStr(VarDecl *var) {
  std::stringstream SSres;
  SSres << "apacMemeBloc__" << var->getNameAsString();
  return SSres.str();
}
inline std::string createCreationString(const struct item_found &item,
                                        const LangOptions &langOpts) {
  if (item.array) {
    return createCreationStringArray(item, langOpts);
  } else {
    return createCreationStringNonArray(item, langOpts);
  }
}