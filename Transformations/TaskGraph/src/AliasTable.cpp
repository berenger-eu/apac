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
        tableValueVar->references.insert(tableValueRef);
        tableValueRef->aliased.insert(tableValueVar);
    }
}
void AliasTable::removeDependencyPtr(const VarDecl* ptr)
{
    if(ptr!=nullptr)
    {
        if(ptrAliasTable.count(ptr)==0)
            llvm::errs()<<"Error: Trying to remove dependency from non existing element\n";
        else
        {
            pointersAliasArg* tableValuePtr = &ptrAliasTable.at(ptr);
            for (const auto& varAliased:tableValuePtr->aliased)
            {
                aliasArg* tableValueVar = getAliasArg(&varAliased->declaration);
                tableValueVar->pointers.erase(tableValuePtr);
            }
            tableValuePtr->aliased.clear();
        }
    }
}
const std::unordered_set<const VarDecl*> AliasTable::getAliases(const VarDecl* v ) const{
    std::unordered_set<const VarDecl*> aliases;
    aliases.insert(v);
    int oldSize=1;
    int newsize=1;
    if(refAliasTable.count(v)!=0)
        llvm::errs()<<v->getName()<<"Found reference alias\n";
    else
        llvm::errs()<<v->getName()<<"Not found reference alias\n";
    do{
        oldSize=aliases.size();
        for(const auto& alias:aliases)
        {
            getReferencesAliases(alias,aliases);
            getPointersAliases(alias,aliases);
            if(refAliasTable.count(alias)!=0)
                for(const auto& ref:refAliasTable.at(alias).aliased)
                    aliases.insert(&ref->declaration);
        }
        newsize=aliases.size();
    }
    while(newsize!=oldSize);
    return aliases;
};
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
