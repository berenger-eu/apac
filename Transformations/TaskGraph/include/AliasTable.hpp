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
struct key_struct {
    const clang::NamedDecl* decl;
    std::vector<int> indices;
    void dump() const {
        // Dump the contents of the key_struct
        llvm::errs() << decl->getNameAsString();
        for (const auto& index : indices) {
            llvm::errs() <<" ["<< index << "]";
        }
        llvm::errs() << "\n";
    }
};
struct key_hash {
    std::size_t operator () (const key_struct p) const {
        auto h1 = std::hash<const clang::NamedDecl*>{}(p.decl);
        auto h2 = 0;
        for (const auto& element : p.indices) {
            h2 ^= std::hash<int>{}(element);
        }
        return h1 ^ h2;
    }
};

struct key_equal {
    bool operator () (const key_struct p1, key_struct p2) const {
        return p1.decl == p2.decl ;//&& p1.indices == p2.indices;
    }
};

typedef std::unordered_map<key_struct, struct aliasArg, key_hash, key_equal> VariableAliasTable;
typedef std::unordered_map<key_struct, struct referenceAliasArg, key_hash, key_equal> ReferenceAliasTable;
typedef std::unordered_map<key_struct, struct pointersAliasArg, key_hash, key_equal> PointersAliasTable;

class AliasTable {
    public:
        AliasTable(Rewriter& R) : TheRewriter(R){}
        const std::unordered_set<const VarDecl*> getAliases(const VarDecl* v ) const;
        std::unordered_set<const VarDecl*> getAliased(const VarDecl* v) ;
        inline void addVariableToTables(const VarDecl* v){
            if(v!=nullptr)
            {
                referenceAliasArg ref{*v};
                pointersAliasArg ptr{*v};
                
                refAliasTable.insert({{getKey(v),std::vector<int>()},ref});
                //   ptrAliasTable.insert({getKey(v),ptr});
            }
        }
        void removeDependencyPtr(const VarDecl* ptr);
        void addAliasReference(const VarDecl* var,const VarDecl* ref);
        void addAliasPtr(const VarDecl* var,const VarDecl* ptr);
        void getModifiedVariables(std::unordered_set<const VarDecl*>& setResults,const int& depth); 
        void inline dump() const
        {
            dumpVarTable();
            dumpRefTable();
            dumpPtrTable();
        };
        inline const aliasArg* getAliasArg(const VarDecl* v) const
        {
            const aliasArg* result=nullptr;
            if(getVarAliasArg(v)!=nullptr)
                result= getVarAliasArg(v);
            else if(getPtrAliasArg(v)!=nullptr)
                result= getPtrAliasArg(v);
            else if(getRefAliasArg(v)!=nullptr)
                result= getRefAliasArg(v);
            return result;
        }
        inline const aliasArg* getVarAliasArg(const VarDecl* v) const {
            const NamedDecl* result=getKey(v);
            if(varAliasTable.count({result,std::vector<int>()})==0)
                return nullptr;
            return &varAliasTable.at({result,std::vector<int>()});
        }
        inline const pointersAliasArg* getPtrAliasArg(const VarDecl* v) const {
            const NamedDecl* result=getKey(v);
            if(ptrAliasTable.count({result,std::vector<int>()})==0)
                return nullptr;
            return &ptrAliasTable.at({result,std::vector<int>()});
        }
        inline const referenceAliasArg* getRefAliasArg(const VarDecl* v) const {
            const NamedDecl* result=getKey(v);
            if(refAliasTable.count({result,std::vector<int>()})==0)
                return nullptr;
            return &refAliasTable.at({result,std::vector<int>()});
        }
    private:
        inline const NamedDecl* getKey(const VarDecl* v) const{
            return v->getCanonicalDecl();
        }
        
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
            if(varAliasTable.count({result,std::vector<int>()})==0)
                return nullptr;
            return &varAliasTable.at({result,std::vector<int>()});
        }
        
        inline pointersAliasArg* getPtrAliasArg(const VarDecl* v) {
            const NamedDecl* result=getKey(v);
            if(ptrAliasTable.count({result,std::vector<int>()})==0)
                return nullptr;
            return &ptrAliasTable.at({result,std::vector<int>()});
        }
        
        inline referenceAliasArg* getRefAliasArg(const VarDecl* v)  {
             const NamedDecl* result=getKey(v);
            if(refAliasTable.count({result,std::vector<int>()})==0)
                return nullptr;
            return &refAliasTable.at({result,std::vector<int>()});
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

