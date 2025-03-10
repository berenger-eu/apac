#include "core.hpp"
using namespace clang;
/*
// Creates a string for the creation of an array (type (*varName)[]...  = new
// type[][]... ) Format :  type (*varName)[]...  = new type[][]... );
//           type (*&varName)[]...= apacMem;
std::string createCreationStringArray(const struct item_found &,
                                      const LangOptions &);
// Creates a string for the creation of an non array variable
// Format :  type *const apacMem=new type;
//           type& varName= *(apacMem);
std::string createCreationStringNonArray(const struct item_found &,
                                         const LangOptions &);

// Inline functions


//
*/