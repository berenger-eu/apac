#include "../include/stringManipulation.hpp"
using namespace clang;
std::string getCompleteVarDeclStr(VarDecl& v)
{
  std::stringstream SSresult;
  //type varName
  SSresult<<v.getType().getAsString()<<" "<<v.getNameAsString();
  if(v.getInit())
  {
    //= initValue
    SSresult<<" = "<<createInitString(v);
  }
  SSresult<<";\n";
  return SSresult.str();
}

//returns the string containing the Init part of a variable (Variable is supposed to be init here)
std::string createInitString(VarDecl& v)
{
  assert(v.getInit()!=NULL);
  PrintingPolicy print_policy(v.getASTContext().getLangOpts());
  std::string initString;
  llvm::raw_string_ostream stringStreamInit(initString);
  v.getInit()->printPretty(stringStreamInit,NULL,print_policy);
  return initString;
}

//Builds the string to delete a variable
std::string createDeleteString(struct item_found& item)
{
  std::stringstream SSprint;
  SSprint<<"delete ";
  if(item.array)
    SSprint<<"[] ";
  SSprint<<getApacMemBlockStr(item)<<";\n";
  return SSprint.str();
}

std::string createDeleteSegment(std::vector<item_found>& itemsToDelete)
{
  std::stringstream SSprint;
  //We iterate over all elements that have to be deleted 
  //and create the corresponding text part
  for(auto b =itemsToDelete.begin(); b!=itemsToDelete.end();b++)
    SSprint<<createDeleteString(*b);
  return SSprint.str();
}

std::string createCreationStringArray(struct item_found& itFound)
{
  std::stringstream SSprint;
  VarDecl& v=*(itFound.declaration);
  std::string strTempMemType=itFound.qTypeTempMem.getAsString();
  std::string strNewType=itFound.qTypeNew.getAsString();
  std::string strVarType=itFound.qTypeVar.getAsString();
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
    SSprint<<createInitString(v);
  
  strStart=strVarType;
  if (found!=std::string::npos)
    strStart.insert(found+1,itFound.name);
  
  SSprint<<";\n"<<
  strStart<<"= ("<<apacMemBloc<<");\n";
  return SSprint.str();
}

std::string createCreationStringNonArray(struct item_found& itFound)
{
  std::stringstream SSprint;
  VarDecl& v=*(itFound.declaration);
  std::string strTempMemType=itFound.qTypeTempMem.getAsString();
  std::string strNewType=itFound.qTypeNew.getAsString();
  std::string strVarType=itFound.qTypeVar.getAsString();
  std::string apacMemBloc=getApacMemBlockStr(itFound);

  SSprint<<strTempMemType<<' '<<apacMemBloc<<" = new "<<strNewType;
  if(v.getInit()!=NULL)
    SSprint<<'('<<createInitString(v)<<')';
  else
    SSprint<<"()";

  SSprint<<";\n"<<
  strVarType<<v.getNameAsString()<<"= *("<<apacMemBloc<<");\n";
  return SSprint.str();
}
