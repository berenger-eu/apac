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
    if (isVariantSubArray(map.at(indexes[0])))
      return getVariantSubArray(map.at(indexes[0]))
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
    if (isVariantSubArray(map.at(indexes[0])))
      return std::get<std::shared_ptr<IndexTableMapStruct>>(map.at(indexes[0]))
          ->at(std::vector<int>(indexes.begin() + 1, indexes.end()));
  }
  // Otherwise, the value is simple variable and we're trying to access an
  // index, so we return nullptr
  return nullptr;
}
int IndexTableMapStruct::nbElements() const {
  int res = 0;
  if (aliasStruct != nullptr)
    res += aliasStruct->nbElements();
  for (auto &elem : map) {
    if (isVariantAliasArgStruct(elem.second))
      res += getVariantAliasArgStruct(elem.second)->nbElements();
    else if (isVariantSubArray(elem.second))
      res += getVariantSubArray(elem.second)->nbElements();
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
  if (isVariantSubArray(map.at(indexes[0])))
    return getVariantSubArray(map.at(indexes[0]))
        ->count(std::vector<int>(indexes.begin() + 1, indexes.end()));
  return 0;
}

void IndexTableMapStruct::insert(
    const std::pair<aliasArg, std::vector<int> &> pair) {
  auto elem = std::make_shared<aliasArg>(pair.first);
  auto indexes = pair.second;
  auto elemArgStruct = std::make_shared<aliasArgStruct>();
  if (elem->hasUnknownIndex)
    elemArgStruct->unknownIndexAliases.insert({elem->getIndexKey(), elem});
  else
    elemArgStruct->alias = elem;
  if (indexes.empty()) {
    if (aliasStruct == nullptr) {
      aliasStruct = elemArgStruct;
    }
  } else {
    auto curMap = &map;
    for (long unsigned int i = 0; i < indexes.size() - 1; i++) {
      if (curMap->count(indexes[i]) == 0)
        curMap->insert({indexes[i], std::make_shared<IndexTableMapStruct>()});
      else if (isVariantAliasArgStruct(curMap->at(indexes[i]))) {
        auto element = std::make_shared<IndexTableMapStruct>();
        element->aliasStruct = getVariantAliasArgStruct(curMap->at(indexes[i]));
        curMap->at(indexes[i]) = element;
      }
      if (isVariantSubArray(curMap->at(indexes[i])))
        curMap = &getVariantSubArray(curMap->at(indexes[i]))->map;
    }
    if (curMap->count(indexes.back()) == 0)
      curMap->insert({indexes.back(), elemArgStruct});
    else if (isVariantAliasArgStruct(curMap->at(indexes.back()))) {
      auto element = std::make_shared<IndexTableMapStruct>();
      element->aliasStruct =
          getVariantAliasArgStruct(curMap->at(indexes.back()));
      curMap->at(indexes.back()) = element;
    }
    if (isVariantSubArray(curMap->at(indexes.back())) &&
        elemArgStruct != nullptr)
      *getVariantSubArray(curMap->at(indexes.back()))->aliasStruct +=
          *elemArgStruct;
  }
}
void IndexTableMapStruct::dumpPrep(std::string *varTable, std::string *refTable,
                                   std::string *ptrTable) const {
  std::stringstream ssVar, ssRef, ssPtr;
  // The node might not be linked to an alias
  // Example : To access tab [0][1], we need to access tab[0] first but tab[0]
  // might not have an alias
  if (aliasStruct != nullptr) {
    auto alias = aliasStruct->alias;
    dumpPrepAliasStruct(aliasStruct, ssVar, ssRef, ssPtr);
  }
  for (auto &elem : map) {
    if (isVariantAliasArgStruct(elem.second)) {
      auto aliasStruct = getVariantAliasArgStruct(elem.second);
      dumpPrepAliasStruct(aliasStruct, ssVar, ssRef, ssPtr);
    } else if (isVariantSubArray(elem.second)) {
      auto subIndexMapStruct = getVariantSubArray(elem.second);
      subIndexMapStruct->dumpPrep(varTable, refTable, ptrTable);
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