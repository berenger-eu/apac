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
    const AliasType type;
    //Elements that point to current element
    std::unordered_set<pointersAliasArg*> pointers;
    //Elements that references current element
    std::unordered_set<referenceAliasArg*> references;
    aliasArg(const clang::VarDecl& decl, AliasType t):declaration(decl),type(t){}
    virtual void dump() const;

};

struct pointersAliasArg : public aliasArg
{
    pointersAliasArg(const clang::VarDecl& decl):aliasArg{decl,Pointer}{}
    //Element referenced
    std::unordered_set<aliasArg*> aliased;
    void dump() const override;
};
struct referenceAliasArg : public aliasArg
{
    referenceAliasArg(const clang::VarDecl& decl):aliasArg{decl,Reference}{}
    //Element referenced
    std::unordered_set<aliasArg*> aliased;
    void dump() const override;
};
//Three tables, one for each type of data (variable,pointers and references)
//It would be possible to only use of table for each type
typedef std::unordered_map<const clang::NamedDecl*, struct aliasArg> VariableAliasTable;
typedef std::unordered_map<const clang::NamedDecl*, struct referenceAliasArg> ReferenceAliasTable;
typedef std::unordered_map<const clang::NamedDecl*, struct pointersAliasArg> PointersAliasTable;

class AliasTable {
    public:
        AliasTable(Rewriter& R) : TheRewriter(R){}
        //Retrieves the variables that are aliases of the given variable
        const std::unordered_set<const VarDecl*> getAliases(const VarDecl* v ) const;
        //Retrives the variable that are aliased by the given variable
        std::unordered_set<const VarDecl*> getAliased(const VarDecl* v) ;
        
        inline void addVariableToTables(const VarDecl* v){
            if(v!=nullptr)
            {
                referenceAliasArg ref{*v};
                pointersAliasArg ptr{*v};
                
                refAliasTable.insert({getKey(v),ref});
                //   ptrAliasTable.insert({getKey(v),ptr});
            }
        }
        //Removes dependencies of the given variable
        //So it will empty its list of aliased variables (and those variables will remove the link to the given variable)
        void removeDependencyPtr(const VarDecl* ptr);
        //Adds an alias because of a reference
        void addAliasReference(const VarDecl* var,const VarDecl* ref);
        //Adds an alias because of a pointer
        void addAliasPtr(const VarDecl* var,const VarDecl* ptr);
        //Takes variables in setResults and returns in setResults variables that would be modified by an access of the given depth
        //Example: if depth is 0, it will return the same variables,their references (and if it's a reference, the aliased variables)
        //For a depth of 1, it will act as with depth 0 but with the pointed variables (so *p, references to it, and referenced variables if any) 
        void getModifiedVariables(std::unordered_set<const VarDecl*>& setResults,const int& depth); 
        void inline dump() const
        {
            dumpVarTable();
            dumpRefTable();
            dumpPtrTable();
        };

    private:
        //Returns a key from a variable declaration
        inline const NamedDecl* getKey(const VarDecl* v) const{
            return v->getCanonicalDecl();
        }
        //Returns the element from the table that corresponds to the given variable
        inline aliasArg* getAliasArg(const VarDecl* v)
        {
            aliasArg* result=nullptr;
            if(getVarAliasArg(v)!=nullptr)
                result= getVarAliasArg(v);
            else if(getPtrAliasArg(v)!=nullptr)
                result= getPtrAliasArg(v);
            else if(getRefAliasArg(v)!=nullptr)
                result= getRefAliasArg(v);
            return result;
        }
        inline aliasArg* getVarAliasArg(const VarDecl* v) {
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
        void dumpPtrTable() const;
        void dumpRefTable() const;
        void dumpVarTable() const;
        VariableAliasTable varAliasTable;
        ReferenceAliasTable refAliasTable;
        PointersAliasTable ptrAliasTable;
        Rewriter& TheRewriter;
};

