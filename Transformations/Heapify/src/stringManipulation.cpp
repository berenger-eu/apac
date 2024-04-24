#include "stringManipulation.hpp"
using namespace clang;

//Builds the string to delete a variable
std::string createDeleteString(const struct item_found& item)
{
  std::stringstream SSprint;
  SSprint<<"delete ";
  if(item.array)
    SSprint<<"[] ";
  SSprint<<getApacMemBlockStr(item)<<";\n";
  return SSprint.str();
}

std::string createDeleteSegment(const std::vector<item_found>& itemsToDelete)
{
  std::stringstream SSprint;
  //We iterate over all elements that have to be deleted 
  //and create the corresponding text part
  for(auto b =itemsToDelete.begin(); b!=itemsToDelete.end();b++)
    SSprint<<createDeleteString(*b);
  return SSprint.str();
}

std::string createCreationStringArray(const struct item_found& itFound,const LangOptions& langOpts)
{
  std::stringstream SSprint;
  VarDecl& v=*(itFound.declaration);
  std::string strTempMemType=itFound.qTypeTempMem.getAsString(langOpts);
  std::string strNewType=itFound.qTypeNew.getAsString(langOpts);
  std::string strVarType=itFound.qTypeVar.getAsString(langOpts);
  std::string apacMemBloc=getApacMemBlockStr(itFound);
  
  //strVarType adds a '&' before where the name of the variable should be
  //So using the its index, we can find where to place the variable name
  //in strTempMemType which is the same string as strVarType except for the '&'
  std::string strStart(strTempMemType);
  std::size_t found = strVarType.find('&');
  if (found!=std::string::npos)
    strStart.insert(found,' '+apacMemBloc);
  
  SSprint<<strStart<<" = new "<<strNewType;
  if(v.getInit()!=NULL)
    SSprint<<getInitString(v);
  
  strStart=strVarType;
  if (found!=std::string::npos)
    strStart.insert(found+1,itFound.name);
  
  SSprint<<";\n"<<
  strStart<<"= ("<<apacMemBloc<<");\n";
  return SSprint.str();
}

std::string createCreationStringNonArray(const struct item_found& itFound,const LangOptions& langOpts)
{
  std::stringstream SSprint;
  VarDecl& v=*(itFound.declaration);
  std::string strTempMemType=itFound.qTypeTempMem.getAsString(langOpts);
  std::string strNewType=itFound.qTypeNew.getAsString(langOpts);
  std::string strVarType=itFound.qTypeVar.getAsString(langOpts);
  std::string apacMemBloc=getApacMemBlockStr(itFound);

  SSprint<<strTempMemType<<' '<<apacMemBloc<<" = new "<<strNewType;
  if(v.getInit()!=NULL)
    SSprint<<'('<<getInitString(v)<<')';
  else
    SSprint<<"()";

  SSprint<<";\n"<<
  strVarType<<v.getNameAsString()<<"= *("<<apacMemBloc<<");\n";
  return SSprint.str();
}
