#include "core.hpp"
using namespace clang;


//Creates a string for the deletion of a variable (delete <varName>)
std::string createDeleteString(const struct item_found& v);
//Creates a segment containing all the delete strings
std::string createDeleteSegment(const std::vector<item_found>& itemsToDelete);
//Creates a string for the creation of an array (type (*varName)[]...  = new type[][]... )
//Format :  type (*varName)[]...  = new type[][]... );
//          type (*&varName)[]...= apacMem; 
std::string createCreationStringArray(const struct item_found& );
//Creates a string for the creation of an non array variable
//Format :  type *const apacMem=new type;
//          type& varName= *(apacMem);
std::string createCreationStringNonArray(const struct item_found& );


//Inline functions

//Builds and returns the string : "apacMemeBloc__varName_varId"
inline std::string getApacMemBlockStr(const struct item_found& item)
{
    std::stringstream SSres;
    SSres<<"apacMemeBloc__"<<item.name<<'_'<<item.uid;
    return SSres.str();
}
inline std::string createCreationString(const struct item_found& item)
{
    if (item.array)
        return createCreationStringArray(item);
    else
        return createCreationStringNonArray(item);
}
//