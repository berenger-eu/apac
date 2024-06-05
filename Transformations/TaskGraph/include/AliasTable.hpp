#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "clang/AST/Decl.h"
#include "clang/Rewrite/Core/Rewriter.h"
using namespace clang;

struct pointersAliasArg;
struct referenceAliasArg;
struct aliasArg {
    clang::VarDecl& declaration;
    //Elements that point to current element
    std::vector<pointersAliasArg&> pointers;
    //Elements that references current element
    std::vector<referenceAliasArg&> references;
};

struct pointersAliasArg : public aliasArg
{
    //Element referenced
    std::vector<>& aliased;
};
struct referenceAliasArg : public aliasArg
{
    //Element referenced
    std::vector<>& aliased;
};

typedef std::unordered_map<clang::NamedDecl*, struct varAliasTable> VariableAliasTable;
typedef std::unordered_map<clang::NamedDecl*, struct referenceAliasArg> ReferenceAliasTable;
typedef std::unordered_map<clang::NamedDecl*,struct pointersAliasArg> PointersAliasTable;
class AliasTable {
    public:
        AliasTable(Rewriter& R) : TheRewriter(R){}
        inline const std::unordered_set<const VarDecl&> getAliases(const VarDecl& v ) const{
            std::unordered_set<const VarDecl&> aliases;
            getReferencesAliases(v,aliases);
            getPointersAliases(v,aliases);
            return aliases;
        };

        inline void addVariableToTables(const VarDecl* v){
            if(v!=nullptr)
            {
                referenceAliasTable.insert({getKey(*v),referenceAliasArg{v}});
                pointersAliasTable.insert({getKey(*v),pointersAliasArg{v}});
            }
        }
        void addAliasReference(VarDecl* var,VarDecl* ref);
    private:
        inline NamedDecl* getKey(const VarDecl* v) const{
            return v.getCanonicalDecl();
        }
        void getReferencesAliases(const VarDecl&,std::unordered_set<const VarDecl&>&) const;
        void getPointersAliases(const VarDecl&,std::unordered_set<const VarDecl&>&) const;
        VariableAliasTable varAliasTable;
        ReferenceAliasTable refAliasTable;
        PointersAliasTable ptrAliasTable;
        Rewriter& TheRewriter;
};
