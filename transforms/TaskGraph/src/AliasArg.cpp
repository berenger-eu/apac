#include "AliasArg.hpp"
int aliasArg::curId = 0;

std::string aliasArg::varAsString() const {
  std::stringstream ssRes;
  // Globals must remain unambiguous at call sites, including namespaces
  // and local variables that shadow a global name.
  if (declaration.isFileVarDecl()) {
    std::string qualified;
    PrintingPolicy policy(declaration.getASTContext().getLangOpts());
    policy.SuppressUnwrittenScope = true; // Anonymous namespaces are not spellable.
    llvm::raw_string_ostream stream(qualified);
    declaration.printQualifiedName(stream, policy);
    ssRes << "::" << stream.str();
  } else {
    ssRes << declaration.getNameAsString();
  }
  if (!hasUnknownIndex)
    for (const auto &index : indexes)
      ssRes << "[" << index << "]";
  else
    ssRes << indexString;
  return ssRes.str();
}

std::string aliasArg::dependencyAsString() const {
  std::string locator = varAsString();
  if (!indexes.empty() && !declaration.getType()->isReferenceType() &&
      isReferenceQualType(declaration.getType())) {
    // OpenMP array locators require an actual pointer/array base. Unlike
    // ordinary C++ indexing, they do not convert reference_wrapper implicitly.
    locator.insert(locator.find('['), ".get()");
  }
  return locator;
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
