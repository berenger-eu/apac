#include "core.hpp"
//Returns a string containing the complete instruction for the declaration of a variable
//Used mostly for multiple declaration (since it is broken down in multiple instructions)
std::string getCompleteVarDeclStr(clang::VarDecl& );
//Creates a string to store the initialisation of a variable 
std::string createInitString(clang::VarDecl& );
//Creates a string for the deletion of a variable (delete ...)
std::string createDeleteString(struct item_found& v);
//Creates a segment containing all the delete strings
std::string createDeleteSegment(std::vector<item_found>& itemsToDelete);
//Creates a string for the creation of a variable (type* = new type)
std::string createCreationString(struct item_found& );


