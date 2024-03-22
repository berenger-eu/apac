#include "core.hpp"
using namespace clang;


//Returns a string containing the complete instruction for the declaration of a variable
//Format: "type varName [= initValue];\n"
//Used mostly for multiple declarations (since they are broken down in multiple instructions)
std::string getCompleteVarDeclStr(VarDecl& );
//Creates a string to store the initialisation of a variable 
std::string createInitString(VarDecl& );
//Creates a string for the deletion of a variable (delete ...)
std::string createDeleteString(struct item_found& v);
//Creates a segment containing all the delete strings
std::string createDeleteSegment(std::vector<item_found>& itemsToDelete);
//Creates a string for the creation of an array (type (*varName)[]...  = new type[][]... )
//Format :  type (*varName)[]...  = new type[][]... );
//          type (*&varName)[]...= apacMem; 
std::string createCreationStringArray(struct item_found& );
//Creates a string for the creation of an non array variable
//Format :  type *const apacMem=new type;
//          type& varName= *(apacMem);
std::string createCreationStringNonArray(struct item_found& );


//Inline functions

//Builds and returns the string : "apacMemeBloc__varName_varId"
inline std::string getApacMemBlockStr(struct item_found& item)
{
    std::stringstream SSres;
    SSres<<"apacMemeBloc__"<<item.name<<'_'<<item.uid;
    return SSres.str();
}
inline std::string createCreationString(struct item_found& item)
{
    if (item.array)
        return createCreationStringArray(item);
    else
        return createCreationStringNonArray(item);
}
//