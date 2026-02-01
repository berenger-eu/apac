#include "AliasTableMaps.hpp"

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
  else if (isVariantAliasArgStruct(map.at(key)))
    llvm::errs() << "No indexes found in table\n";
  // The value is an array, so we look through it using indexes
  else if (isVariantSubArray(map.at(key))) {

    result = getVariantSubArray(map.at(key))->at(indexes);
  }
  return result;
}
const aliasesTableValues *
AliasTableMapStruct::at(const NamedDecl *key,
                        const std::vector<int> &indexes) const {
  const aliasesTableValues *result = nullptr;
  // If no entry for variable, return nullptr
  if (map.count(key) == 0)
    return result;
  // If no indexes, return the value
  if (indexes.empty()) {
    result = &map.at(key);
  }
  // If the value is an aliasArg (and not an array, so no indexes), return
  // nullptr
  else if (isVariantAliasArgStruct(map.at(key)))
    return result;
  // The value is an array, so we look through it using indexes
  else if (isVariantSubArray(map.at(key)))
    result = getVariantSubArray(map.at(key))->at(indexes);
  return result;
}
int AliasTableMapStruct::nbElements() const {
  int res = 0;
  for (auto &elem : map) {
    if (isVariantAliasArgStruct(elem.second))
      res += getVariantAliasArgStruct(elem.second)->nbElements();
    else if (isVariantSubArray(elem.second))
      res += getVariantSubArray(elem.second)->nbElements();
  }
  return res;
}
int AliasTableMapStruct::count(const NamedDecl *key,
                               const std::vector<int> &indexes) const {
  if (indexes.empty())
    return map.count(key);
  else if (map.count(key) != 0) {
    if (isVariantAliasArgStruct(map.at(key)))
      return 0;
    else if (isVariantSubArray(map.at(key))) {
      return getVariantSubArray(map.at(key))->count(indexes);
    }
  }
  return 0;
}
void AliasTableMapStruct::insert(
    const std::pair<aliasArg, std::vector<int> &> pair) {
  auto elem = std::make_shared<aliasArg>(pair.first);
  auto elemAliasStruct = std::make_shared<aliasArgStruct>();
  if (elem->hasUnknownIndex)
    elemAliasStruct->unknownIndexAliases.insert({elem->indexString, elem});
  else
    elemAliasStruct->alias = elem;
  auto indexes = pair.second;
  const NamedDecl *key = elem->declaration.getCanonicalDecl();

  if (indexes.empty()) {
    if (map.count(key) == 0)
      map.insert({key, elemAliasStruct});
    else if (isVariantSubArray(map.at(key))) {
      auto element = getVariantSubArray(map.at(key));
      if (element->aliasStruct == nullptr)
        element->aliasStruct = elemAliasStruct;
    }
  }
  // When adding an array element to the table
  else {

    if (map.count(key) == 0)
      map.insert({key, std::make_shared<IndexTableMapStruct>()});
    else if (isVariantAliasArgStruct(map.at(key))) {
      auto element = std::make_shared<IndexTableMapStruct>();
      element->aliasStruct = getVariantAliasArgStruct(map.at(key));
      map.at(key) = element;
    }
    if (isVariantSubArray(map.at(key)))
      getVariantSubArray(map.at(key))->insert(pair);
  }
}
