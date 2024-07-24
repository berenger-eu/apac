#include "AliasTableMaps.hpp"

aliasesTableValues *IndexTableMapStruct::at(const std::vector<int> &indexes) {
  // If no indexes, return nullptr
  if (indexes.empty())
    return nullptr;
  // If only one index, return the value
  if (indexes.size() == 1 && map.count(indexes[0]))
    return &map.at(indexes[0]);
  // If the value is an IndexTableMapStruct, we look through it using indexes
  else if (map.count(indexes[0])) {
    if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
            map.at(indexes[0])))
      return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(indexes[0]))
          ->at(std::vector<int>(indexes.begin() + 1, indexes.end()));
  }
  // Otherwise, the value is simple variable and we're trying to access an
  // index, so we return nullptr
  return nullptr;
}
const aliasesTableValues *
IndexTableMapStruct::at(const std::vector<int> &indexes) const {
  // If no indexes, return nullptr
  if (indexes.empty())
    return nullptr;
  // If only one index, return the value
  if (indexes.size() == 1 && map.count(indexes[0]))
    return &map.at(indexes[0]);
  // If the value is an IndexTableMapStruct, we look through it using indexes
  else if (map.count(indexes[0])) {
    if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
            map.at(indexes[0])))
      return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(indexes[0]))
          ->at(std::vector<int>(indexes.begin() + 1, indexes.end()));
  }
  // Otherwise, the value is simple variable and we're trying to access an
  // index, so we return nullptr
  return nullptr;
}
int IndexTableMapStruct::nbElements() const {
  int res = 0;
  if (alias != nullptr)
    res++;
  for (auto &elem : map) {
    if (std::holds_alternative<std::shared_ptr<aliasArg>>(elem.second))
      res++;
    else if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
                 elem.second))
      res += std::get<std::shared_ptr<IndexTableMapStruct>>(elem.second)
                 ->nbElements();
  }
  return res;
}
int IndexTableMapStruct::count(const std::vector<int> &indexes) const {
  if (indexes.empty())
    return 0;
  if (indexes.size() == 1)
    return (map.count(indexes[0]) > 0);
  if (map.count(indexes[0]) == 0)
    return 0;
  return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(indexes[0]))
      ->count(std::vector<int>(indexes.begin() + 1, indexes.end()));
}

void IndexTableMapStruct::dumpPrep(std::string *varTable, std::string *refTable,
                                   std::string *ptrTable) const {
  std::stringstream ssVar, ssRef, ssPtr;
  if (alias != nullptr) {
    if (alias->type == Variable && varTable != nullptr)
      ssVar << alias->varAsString() << " ";
    else if (alias->type == Reference && refTable != nullptr)
      ssRef << alias->varAsString() << " ";
    else if (alias->type == Pointer && ptrTable != nullptr)
      ssPtr << alias->varAsString() << " ";
  }
  for (auto &elem : map) {
    if (std::holds_alternative<std::shared_ptr<aliasArg>>(elem.second)) {
      auto alias = std::get<std::shared_ptr<aliasArg>>(elem.second);
      if (varTable != nullptr && alias->type == Variable)
        ssVar << alias->varAsString() << " ";
      else if (refTable != nullptr && alias->type == Reference)
        ssRef << alias->varAsString() << " ";
      else if (ptrTable != nullptr && alias->type == Pointer)
        ssPtr << alias->varAsString() << " ";
    } else if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
                   elem.second)) {
      auto alias = std::get<std::shared_ptr<IndexTableMapStruct>>(elem.second);
      alias->dumpPrep(varTable, refTable, ptrTable);
      if (varTable != nullptr)
        ssVar << *varTable;
      if (refTable != nullptr)
        ssRef << *refTable;
      if (ptrTable != nullptr)
        ssPtr << *ptrTable;
    }
  }
  if (varTable != nullptr)
    *varTable = ssVar.str();
  if (refTable != nullptr)
    *refTable = ssRef.str();
  if (ptrTable != nullptr)
    *ptrTable = ssPtr.str();
}
/*

Alias Table (key is NamedDecl, value is either aliasArg or IndexTableMapStruct)



*/

aliasesTableValues *AliasTableMapStruct::at(const NamedDecl *key,
                                            const std::vector<int> &indexes) {
  aliasesTableValues *result = nullptr;
  // If no entry for variable, return nullptr
  if (map.count(key) == 0)
    llvm::errs() << "No entry for variable\n";
  // If no indexes, return the value
  if (indexes.empty()) {
    result = &map.at(key);
  }
  // If the value is an aliasArg (and not an array, so no indexes), return
  // nullptr
  else if (std::holds_alternative<std::shared_ptr<aliasArg>>(map.at(key)))
    llvm::errs() << "No indexes found in table\n";
  // The value is an array, so we look through it using indexes
  else {

    result = std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key))
        ->at(indexes);
  }
  return result;
}
const aliasesTableValues *
AliasTableMapStruct::at(const NamedDecl *key,
                        const std::vector<int> &indexes) const {
  const aliasesTableValues *result = nullptr;
  // If no entry for variable, return nullptr
  if (map.count(key) == 0)
    ;
  // If no indexes, return the value
  if (indexes.empty()) {
    result = &map.at(key);
  }
  // If the value is an aliasArg (and not an array, so no indexes), return
  // nullptr
  else if (std::holds_alternative<std::shared_ptr<aliasArg>>(map.at(key)))
    ;
  // The value is an array, so we look through it using indexes
  else
    result = std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key))
        ->at(indexes);
  return result;
}
int AliasTableMapStruct::nbElements() const {
  int res = 0;
  for (auto &elem : map) {
    if (std::holds_alternative<std::shared_ptr<aliasArg>>(elem.second))
      res++;
    else if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
                 elem.second))
      res += std::get<std::shared_ptr<IndexTableMapStruct>>(elem.second)
                 ->nbElements();
  }
  return res;
}
int AliasTableMapStruct::count(const NamedDecl *key,
                               const std::vector<int> &indexes) const {
  if (indexes.empty())
    return map.count(key);
  else if (map.count(key) != 0) {
    if (std::holds_alternative<std::shared_ptr<aliasArg>>(map.at(key)))
      return 0;
    return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key))
        ->count(indexes);
  }
  return 0;
}
void AliasTableMapStruct::insert(
    const std::pair<aliasArg, std::vector<int> &> pair) {
  auto elem = std::make_shared<aliasArg>(pair.first);
  auto indexes = pair.second;
  const NamedDecl *key = elem->declaration.getCanonicalDecl();

  if (indexes.empty()) {
    if (map.count(key) == 0)
      map.insert({key, elem});
    else if (std::holds_alternative<std::shared_ptr<IndexTableMapStruct>>(
                 map.at(key))) {
      auto element =
          std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key));
      if (element->alias == nullptr)
        element->alias = elem;
    }
  } else if (map.count(key) == 0) {
    map.insert({key, std::make_shared<IndexTableMapStruct>()});
    std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(key))
        ->map.insert({indexes[0], elem});
  }
}
