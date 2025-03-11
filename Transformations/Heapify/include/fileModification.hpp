#pragma once
#include "core.hpp"
#include "utilitaryFunctions.hpp"
using namespace clang;

void modifyFile(const std::unordered_map<CompoundStmt *,
                                         std::shared_ptr<ScopeInfo>> &scopes,
                Rewriter &TheRewriter);
// Creates a string for the deletion of a variable (delete <varName>)
std::string createDeleteString(const std::shared_ptr<struct item_found> &var);
// Creates a segment containing all the delete strings
std::string createDeleteSegment(
    const std::vector<std::shared_ptr<struct item_found>> &varsToDelete);
std::string
createCreationStringNonArray(const std::shared_ptr<struct item_found> &itFound,
                             const LangOptions &langOpts);
std::string
createCreationStringArray(const std::shared_ptr<struct item_found> &itFound,
                          const LangOptions &langOpts);
void initItem(struct item_found &item, VarDecl &vDec);
// Builds and returns the string : "apacMemeBloc__varName_varId"

inline std::string
getApacMemBlockStr(const std::shared_ptr<struct item_found> &var) {
  std::stringstream SSres;
  SSres << "apacMemeBloc__" << var->name << "_" << var->id;
  return SSres.str();
}
inline std::string
createCreationString(const std::shared_ptr<struct item_found> &item,
                     const LangOptions &langOpts) {
  if (item->array) {
    return createCreationStringArray(item, langOpts);
  } else {
    return createCreationStringNonArray(item, langOpts);
  }
}