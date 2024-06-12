#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "clang/AST/Decl.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "common.hpp"
using namespace clang;
enum AliasType{
        Reference,
        Pointer,
        Variable
    };
struct pointersAliasArg;
struct referenceAliasArg;
struct aliasArg {
    const clang::VarDecl& declaration;
    //Type of Alias arg
    AliasType type;
    //Elements that point to current element
    std::unordered_set<pointersAliasArg*> pointers;
    //Elements that references current element
    std::unordered_set<referenceAliasArg*> references;
    void dump() const;

};

struct pointersAliasArg : public aliasArg
{
    pointersAliasArg(const clang::VarDecl& decl):aliasArg{decl,Pointer}{}
    //Element referenced
    std::unordered_set<aliasArg*> aliased;
};
struct referenceAliasArg : public aliasArg
{
    referenceAliasArg(const clang::VarDecl& decl):aliasArg{decl,Reference}{}
    //Element referenced
    std::unordered_set<aliasArg*> aliased;
};

typedef std::unordered_map<const clang::NamedDecl*, struct aliasArg> VariableAliasTable;
typedef std::unordered_map<const clang::NamedDecl*, struct referenceAliasArg> ReferenceAliasTable;
typedef std::unordered_map<const clang::NamedDecl*, struct pointersAliasArg> PointersAliasTable;

class AliasTable {
    public:
        AliasTable(Rewriter& R) : TheRewriter(R){}
        const std::unordered_set<const VarDecl*> getAliases(const VarDecl* v ) const;

        inline void addVariableToTables(const VarDecl* v){
            if(v!=nullptr)
            {
                referenceAliasArg ref{*v};
                pointersAliasArg ptr{*v};
                
                refAliasTable.insert({getKey(v),ref});
                //   ptrAliasTable.insert({getKey(v),ptr});
            }
        }
        void removeDependencyPtr(const VarDecl* ptr);
        void addAliasReference(const VarDecl* var,const VarDecl* ref);
        void addAliasPtr(const VarDecl* var,const VarDecl* ptr);
        void getAliased(std::unordered_set<const VarDecl*>& setResults,const int& depth); 

    private:
        inline const NamedDecl* getKey(const VarDecl* v) const{
            return v->getCanonicalDecl();
        }
        inline aliasArg* getAliasArg(const VarDecl* v) {
            const NamedDecl* result=getKey(v);
            if(varAliasTable.count(result)==0)
                return nullptr;
            return &varAliasTable.at(getKey(v));
        }
        inline pointersAliasArg* getPtrAliasArg(const VarDecl* v) {
            const NamedDecl* result=getKey(v);
            if(ptrAliasTable.count(result)==0)
                return nullptr;
            return &ptrAliasTable.at(getKey(v));
        }
        inline referenceAliasArg* getRefAliasArg(const VarDecl* v) {
            const NamedDecl* result=getKey(v);
            if(refAliasTable.count(result)==0)
                return nullptr;
            return &refAliasTable.at(getKey(v));
        }
        void getReferencesAliases(const VarDecl*,std::unordered_set<const VarDecl*>&) const;
        void getPointersAliases(const VarDecl*,std::unordered_set<const VarDecl*>&) const;
        
        VariableAliasTable varAliasTable;
        ReferenceAliasTable refAliasTable;
        PointersAliasTable ptrAliasTable;
        Rewriter& TheRewriter;
};

