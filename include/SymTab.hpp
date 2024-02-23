#pragma once
#include "core.hpp"
using namespace clang;
class SymTab {
    public:
        SymTab(Rewriter& R) : TheRewriter(R){
            lastMethDecl=NULL;
        };
        CXXMethodDecl* lastMethDecl;
        const_arg* getInnerConstArg(ValueDecl* );
        const_arg* getInnerConstArg(Expr* );
        const_arg* getHashTableValue (NamedDecl* );
        const_arg* getHashTableValue(CXXThisExpr* );
        void addDependencyHashTable(const_arg* ,const_arg*);
        Decl* getHashKey(NamedDecl*);
        TableConstArg const_arg_table;
        TableConstArgExpr const_arg_expr_table;
        TableFileID fileID_table;
    private:
        Rewriter& TheRewriter;
};

