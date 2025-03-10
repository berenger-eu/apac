#include "stringManipulation.hpp"
using namespace clang;

/*
std::string createCreationStringArray(const struct item_found &itFound,
const LangOptions &langOpts) {
  std::stringstream SSprint;
  VarDecl &v = *(itFound.declaration);
  std::string strTempMemType = itFound.qTypeTempMem.getAsString(langOpts);
  std::string strNewType = itFound.qTypeNew.getAsString(langOpts);
  std::string strVarType = itFound.qTypeVar.getAsString(langOpts);
  std::string apacMemBloc = getApacMemBlockStr(itFound);

  // strVarType adds a '&' before where the name of the variable should be
  // So using the its index, we can find where to place the variable name
  // in strTempMemType which is the same string as strVarType except for the '&'
  std::string strStart(strTempMemType);
  std::size_t found = strVarType.find('&');
  if (found != std::string::npos) {
    strStart.insert(found, ' ' + apacMemBloc);
  }

  SSprint << strStart << " = new " << strNewType;
  if (v.getInit() != NULL) {
    SSprint << getInitString(v);
  }

  strStart = strVarType;
  if (found != std::string::npos) {
    strStart.insert(found + 1, itFound.name);
  }

  SSprint << ";\n" << strStart << "= (" << apacMemBloc << ");\n";
  return SSprint.str();
}



*/