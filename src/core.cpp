#include "../include/core.hpp"
using namespace clang;
std::string getHashKey(NamedDecl* nd)
{
    std::stringstream SSConcatStrings;
    if(nd->getDeclContext()->isFunctionOrMethod())
        SSConcatStrings<<cast<FunctionDecl>(nd->getDeclContext())->getNameAsString();
    std::string varName=nd->getQualifiedNameAsString();
    SSConcatStrings<<varName;
    return SSConcatStrings.str();
}