#include "../include/stringManipulation.hpp"
using namespace clang;
std::string getCompleteVarDeclStr(VarDecl& v)
{
  std::stringstream SSresult;
  SSresult<<v.getType().getAsString()<<" "<<v.getNameAsString();
  if(v.getInit())
  {
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
//Remove the reference from a qualtype and returns the result

//Builds the string to delete a variable
std::string createDeleteString(struct item_found& v)
{
  std::stringstream SSprint;
  SSprint<<"delete";
  if(v.array)
    SSprint<<"[] ";
  SSprint<<" apacMemeBloc__"<<v.name<<'_'<<v.uid<<";\n";
  return SSprint.str();
}
std::string createDeleteSegment(std::vector<item_found>& itemsToDelete)
{
  std::stringstream SSprint;
  for(auto b =itemsToDelete.begin(); b!=itemsToDelete.end();b++)
    SSprint<<createDeleteString(*b);
  return SSprint.str();
}

std::string createCreationString(struct item_found& itFound)
{
  std::stringstream SSprint;
  VarDecl& v=*(itFound.declaration);
  std::string strTempMemType=itFound.qTypeTempMem.getAsString();
  std::string strNewType=itFound.qTypeNew.getAsString();
  std::string strVarType=itFound.qTypeVar.getAsString();
  if(itFound.array)
  {
    //type (* varname) = new type[N]
    std::string strStart(strTempMemType);
    std::stringstream SSapacBloc;
    SSapacBloc<<" apacMemeBloc__"<<v.getNameAsString()<<'_'<<itFound.uid;
    std::size_t found = strVarType.find('&');
    //std::size_t found1 = strStart.find('=');

    if (found!=std::string::npos)
      strStart.insert(found,SSapacBloc.str());
    /*SSprint<<v.getType().getTypePtrOrNull()->getAsArrayTypeUnsafe()->getElementType().getAsString()<<"* "
    <<"apacMemeBloc__"<<v.getNameAsString()<<"_"<<itFound.uid<<" = new "<<strNewType;
    */
    SSprint<<strStart<<" = new "<<strNewType;
    if(v.getInit()!=NULL)
      SSprint<<createInitString(v);
    strStart=strVarType;
    //found = strVarType.find('&');
    if (found!=std::string::npos)
      strStart.insert(found+1,itFound.name);
    SSprint<<";\n"<<strStart<<"= (apacMemeBloc__"<<itFound.name<<'_'<<itFound.uid<<");\n";
  }
  else
  {
    SSprint<<strTempMemType<<" apacMemeBloc__"<<itFound.name<<'_'<<itFound.uid<<" = new "
    <<strNewType;
    if(v.getInit()!=NULL)
    {
      SSprint<<'('<<createInitString(v)<<')';
    }
    else
      SSprint<<"()";
    SSprint<<";\n"<<strVarType<<v.getNameAsString()
    <<"= *(apacMemeBloc__"<<itFound.name<<'_'<<itFound.uid<<");\n";
  }
  return SSprint.str();
}
