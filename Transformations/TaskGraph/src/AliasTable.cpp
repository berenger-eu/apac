#include "AliasTable.hpp"

using namespace clang;
/*
void AliasTable::addAliasReference(const VarDecl* var,const VarDecl* ref)
{
    if(var!=nullptr && ref!=nullptr)
    {
        if(refAliasTable.count(ref)==0)
            refAliasTable.insert({ref,referenceAliasArg{declaration=ref,referenced=var}});
        if(varAliasTable.count(var)==0)
            varAliasTable.insert({var,varAliasArg{declaration=var}});
        varAliasTable.at(var).references.push_back(ref);
    }
}
void AliasTable::getReferencesAliases(const VarDecl& v,std::unordered_set<const VarDecl&>& aliases) const{
    if(varAliasTable.count(&v)==0)
        return;
    for(const auto& alias:varAliasTable.at(&v).references)
        aliases.insert(alias);
}
void AliasTable::getPointersAliases(const VarDecl& v,std::unordered_set<const VarDecl&>& aliases) const{
    if(ptrAliasTable.count(&v)==0)
        return;
    for(const auto& alias:ptrAliasTable.at(&v).pointers)
        aliases.insert(alias);
}
*/