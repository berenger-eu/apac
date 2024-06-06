#include "AliasTable.hpp"

using namespace clang;
void AliasTable::addAliasReference(const VarDecl* var,const VarDecl* ref)
{
    if(var!=nullptr && ref!=nullptr)
    {
        if(refAliasTable.count(ref)==0)
            refAliasTable.insert({ref,referenceAliasArg{*ref}});
        if(varAliasTable.count(var)==0)
            varAliasTable.insert({var,aliasArg{*var}});
        referenceAliasArg* tableValueRef = &refAliasTable.at(ref);
        aliasArg* tableValueVar = &varAliasTable.at(var);
        tableValueVar->references.push_back(tableValueRef);
        tableValueRef->aliased.push_back(tableValueVar);
    }
}
void AliasTable::getReferencesAliases(const VarDecl* v,std::unordered_set<const VarDecl*>& aliases) const{
    if(varAliasTable.count(v)==0)
        return;
    for(const auto& alias:varAliasTable.at(v).references)
        aliases.insert(&alias->declaration);
}
void AliasTable::getPointersAliases(const VarDecl* v,std::unordered_set<const VarDecl*>& aliases) const{
    if(ptrAliasTable.count(v)==0)
        return;
    for(const auto& alias:ptrAliasTable.at(v).pointers)
        aliases.insert(&alias->declaration);
}
