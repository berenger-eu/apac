#pragma once
#include "core.hpp"
using namespace clang;

// Non Inline functions

// True when the function should be traversed
// So true when it is the function passed in argument to the program or
// when there is no arguments and all functions should be traversed
bool foundCorrectFunction(Decl &, const std::string &);
// True when the type of the variable is a type that has to be put on heap
//(false for references and pointers for now)
bool foundCorrectVarType(VarDecl &);
// True when the variable can be put on the heap
// So when its type is valid and it's a variable we're looking for
bool foundCorrectVariable(VarDecl &, const std::string &);
// True when the initialization is not a variable
bool isInitAReference(VarDecl &);
// True when there is a new in the initialization
bool isInitNew(VarDecl &);

void computeNeededHeap(
    const std::vector<std::shared_ptr<ScopeInfo>> &topScopes);
// Inline functions
