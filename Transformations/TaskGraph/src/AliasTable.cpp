#include "AliasTable.hpp"

using namespace clang;

void aliasArg::dump() const{
        llvm::errs()<<"AliasArg: "<<declaration.getNameAsString()<<"\n";
        switch(type){
            case Reference:
                llvm::errs()<<"Type: Reference\n";
                llvm::errs()<<"Aliases: ";
                for(const auto& ref:references)
                    llvm::errs()<<ref->declaration.getNameAsString()<<" ";
                llvm::errs()<<"\n";
                break;
            case Pointer:
                llvm::errs()<<"Type: Pointer\n";
                llvm::errs()<<"Aliases: ";
                for(const auto& ptr:pointers)
                    llvm::errs()<<ptr->declaration.getNameAsString()<<" ";
                llvm::errs()<<"\n";
                break;
            case Variable:
                llvm::errs()<<"Type: Variable\n";
                break;
            default:
                llvm::errs()<<"Type: Unknown\n";
                break;
        }
        llvm::errs()<<"Pointers: ";
        for(const auto& ptr:pointers)
            llvm::errs()<<ptr->declaration.getNameAsString()<<" ";
        llvm::errs()<<"\n";
        llvm::errs()<<"References: ";
        for(const auto& ref:references)
            llvm::errs()<<ref->declaration.getNameAsString()<<" ";
        llvm::errs()<<"\n";
    
    }


void AliasTable::addAliasReference(const VarDecl* var,const VarDecl* ref)
{
    if(var!=nullptr && ref!=nullptr)
    {
        if(refAliasTable.count(ref)==0)
            refAliasTable.insert({ref,referenceAliasArg{*ref}});
        if(varAliasTable.count(var)==0)
            varAliasTable.insert({var,aliasArg{*var,AliasType::Variable}});
        referenceAliasArg* tableValueRef = &refAliasTable.at(ref);
        aliasArg* tableValueVar = &varAliasTable.at(var);
        tableValueVar->references.insert(tableValueRef);
        tableValueRef->aliased.insert(tableValueVar);
    }
}
void AliasTable::addAliasPtr(const VarDecl* var,const VarDecl* ptr)
{
    if(var!=nullptr && ptr!=nullptr)
    {
        pointersAliasArg* tableValuePtr;
        aliasArg* tableValueVar;
        if(ptrAliasTable.count(ptr)==0)
            ptrAliasTable.insert({ptr,pointersAliasArg{*ptr}});
        tableValuePtr = &ptrAliasTable.at(ptr);    
        if(isPointerQualType(var->getType()))
        {
            if(ptrAliasTable.count(var)==0)
                ptrAliasTable.insert({var,pointersAliasArg{*var}});
            tableValueVar= &ptrAliasTable.at(var);
        }
        else{
            if(varAliasTable.count(var)==0)
                varAliasTable.insert({var,aliasArg{*var,AliasType::Variable}});
            tableValueVar= &varAliasTable.at(var);
        }
        tableValueVar->pointers.insert(tableValuePtr);
        tableValuePtr->aliased.insert(tableValueVar);
    }
}
void AliasTable::removeDependencyPtr(const VarDecl* ptr)
{
    if(ptr!=nullptr)
    {
        if(ptrAliasTable.count(ptr)!=0)
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
    do{
        oldSize=aliases.size();
        for(const auto& alias:aliases)
        {
            getReferencesAliases(alias,aliases);
            // getPointersAliases(alias,aliases);
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
    if(varAliasTable.count(v)==0)
        return;
    for(const auto& alias:varAliasTable.at(v).pointers)
        aliases.insert(&alias->declaration);
}

void AliasTable::getModifiedVariables(std::unordered_set<const VarDecl*>& setResults,const int& depth) 
{
    llvm::errs()<<"Depth: "<<depth<<"\n";
    if(depth>0)
    {
        llvm::errs()<<"VarTable size: "<<varAliasTable.size()<<"\n";
        for(auto& var:varAliasTable)
        {
            llvm::errs()<<var.first->getNameAsString()<<" var iazvarpp\n";
            llvm::errs()<<(var.second.references.size())<<"\n";
            llvm::errs()<<(var.second.pointers.size())<<"\n";
        }
        llvm::errs()<<"PtrTable size: "<<ptrAliasTable.size()<<"\n";
        for(auto& ptr:ptrAliasTable)
        {
            llvm::errs()<<ptr.first->getNameAsString()<<" ptr iazvarpp\n";
            llvm::errs()<<(ptr.second.type==Pointer)<<"\n";
        }
        llvm::errs()<<"RefTable size: "<<refAliasTable.size()<<"\n";
        for(auto& ref:refAliasTable)
        {
            llvm::errs()<<ref.first->getNameAsString()<<" ref iazvarpp\n";
            llvm::errs()<<(ref.second.type==Reference)<<"\n";
        }

        int curDepth=0;
        std::unordered_set<aliasArg*> curSet,precSet;
        for(auto& dep:setResults)
        {
             curSet.insert(getPtrAliasArg(dep));
            llvm::errs()<<(getAliasArg(dep)==nullptr)<<"\n";
            llvm::errs()<<(getPtrAliasArg(dep)==nullptr)<<"\n";
            llvm::errs()<<(getRefAliasArg(dep)==nullptr)<<"\n";
        }
        llvm::errs()<<"Error: Depth must be greater than 0\n";


        while(curDepth<depth)
        {
            precSet=curSet;
            curSet.clear();
            llvm::errs()<<"precSetSize"<<precSet.size()<<"\n";
            
            for(auto& dep:precSet)
            {
            llvm::errs()<<(dep->type==Pointer)<<"\n";
                if(dep->type==Reference)
                {
                    referenceAliasArg* ref = static_cast<referenceAliasArg*>(dep);
                    llvm::errs()<<ref->declaration.getName()<<"isRef\n";
                    /*for (auto aliasedVar:ref->aliased)
                    {
                        llvm::errs()<<"test"<<"\n";  
                        curSet.insert(aliasedVar);
                    }*/
                }    
                else if(dep->type==Pointer)
                {
                    pointersAliasArg* dep2 = static_cast<pointersAliasArg*>(dep);
                    curSet.insert(dep2->aliased.begin(),dep2->aliased.end());
                }
            }
            
            curDepth++;
        }
        setResults.clear();
        for(auto& dep:curSet)
        {
            llvm::errs()<<dep->declaration.getNameAsString()<<" dep\n";
            setResults.insert(&dep->declaration);
        }
    } 
}

void AliasTable::dumpVarTable() const
{
    llvm::errs()<<"Variable Table\n\n";
    for(auto& var:varAliasTable)
    {
        llvm::errs()<<var.first->getNameAsString()<<" var\n";
        var.second.dump();
    }
    llvm::errs()<<"\n";

}
void AliasTable::dumpRefTable() const
{
    llvm::errs()<<"Reference Table\n\n";
    for(auto& ref:refAliasTable)
    {
        llvm::errs()<<ref.first->getNameAsString()<<" ref\n";
        ref.second.dump();
    }
    llvm::errs()<<"\n";
}
void AliasTable::dumpPtrTable() const
{
    llvm::errs()<<"Pointer Table\n\n";
    for(auto& ptr:ptrAliasTable)
    {
        llvm::errs()<<ptr.first->getNameAsString()<<" ptr\n";
        ptr.second.dump();
    }
    llvm::errs()<<"\n";

}