#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "clang/AST/Decl.h"
#include "clang/Rewrite/Core/Rewriter.h"
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
    std::vector<pointersAliasArg*> pointers;
    //Elements that references current element
    std::vector<referenceAliasArg*> references;
};

struct pointersAliasArg : public aliasArg
{
    pointersAliasArg(const clang::VarDecl& decl):aliasArg{decl,Pointer}{}
    //Element referenced
    std::vector<aliasArg*> aliased;
};
struct referenceAliasArg : public aliasArg
{
    referenceAliasArg(const clang::VarDecl& decl):aliasArg{decl,Reference}{}
    //Element referenced
    std::vector<aliasArg*> aliased;
};

typedef std::unordered_map<const clang::NamedDecl*, struct aliasArg> VariableAliasTable;
typedef std::unordered_map<const clang::NamedDecl*, struct referenceAliasArg> ReferenceAliasTable;
typedef std::unordered_map<const clang::NamedDecl*, struct pointersAliasArg> PointersAliasTable;

class AliasTable {
    public:
        AliasTable(Rewriter& R) : TheRewriter(R){}
        inline const std::unordered_set<const VarDecl*> getAliases(const VarDecl* v ) const{
            std::unordered_set<const VarDecl*> aliases;
            getReferencesAliases(v,aliases);
            getPointersAliases(v,aliases);
            return aliases;
        };

        
        inline void addVariableToTables(const VarDecl* v){
            if(v!=nullptr)
            {
                referenceAliasArg ref{*v};
                pointersAliasArg ptr{*v};
                
                refAliasTable.insert({getKey(v),ref});
                //   ptrAliasTable.insert({getKey(v),ptr});
            }
        }
        
        void addAliasReference(const VarDecl* var,const VarDecl* ref);
    private:
        inline const NamedDecl* getKey(const VarDecl* v) const{
            return v->getCanonicalDecl();
        }
        
        void getReferencesAliases(const VarDecl*,std::unordered_set<const VarDecl*>&) const;
        void getPointersAliases(const VarDecl*,std::unordered_set<const VarDecl*>&) const;
        
        VariableAliasTable varAliasTable;
        ReferenceAliasTable refAliasTable;
        PointersAliasTable ptrAliasTable;
        Rewriter& TheRewriter;
};

