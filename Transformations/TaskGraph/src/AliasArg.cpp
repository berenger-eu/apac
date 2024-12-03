#include "AliasArg.hpp"
int aliasArg::curId = 0;

std::string aliasArg::varAsString() const {
  std::stringstream ssRes;
  ssRes << declaration.getNameAsString();
  if (!hasUnknownIndex)
    for (const auto &index : indexes)
      ssRes << "[" << index << "]";
  else
    ssRes << indexString;
  return ssRes.str();
}

std::string aliasArg::dumpAsStr() const {
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
