#pragma once
#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "PotTaskGraphInterface.hpp"
#include "common.hpp"
#include "AliasTable.hpp"

using namespace clang;
class ASTTaskGraphVisitor : public RecursiveASTVisitor<ASTTaskGraphVisitor>
{
public:
    ASTTaskGraphVisitor(Rewriter &R) : TheRewriter(R),aliasTable(R) {};
    inline bool VisitStmt(Stmt *s){return true;}
    //Traverse methods lets us stop visiting nodes that we don't need

    inline bool TraverseCXXMethodDecl(CXXMethodDecl *m){
        return TraverseFunctionDecl(m);
    }
    bool TraverseFunctionDecl(FunctionDecl *f);
    //Calls respective handle method

    inline bool TraverseCXXMemberCallExpr(CXXMemberCallExpr* c){
        return traverseSimpleElements(c);
    }
    inline bool TraverseCallExpr(CallExpr* c){
        return traverseSimpleElements(c);
    }
    inline bool TraverseUnaryOperator(UnaryOperator* uop){
        return traverseSimpleElements(uop);
    }
    inline bool TraverseBinaryOperator(BinaryOperator* bop){
        return traverseSimpleElements(bop);
    }
    inline bool TraverseCompoundAssignOperator(CompoundAssignOperator* bop){
        return traverseSimpleElements(bop);
    }
    inline bool TraverseReturnStmt(ReturnStmt* r){
        return traverseSimpleElements(r);
    }
    inline bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr* c){
        return traverseSimpleElements(c);
    }

    bool TraverseForStmt(ForStmt* f);
    bool TraverseIfStmt(IfStmt* i);
    const auto& getTaskGraphs(){return functionsInstructionsVector;}
    std::vector<std::vector<Instruction>> functionsInstructionsVector;

    const AliasTable& getAliasTable() const{return aliasTable;}
private:
    bool isEmptyInstruction(const Instruction& instr){return instr.dependencies.size()==0;};
    inline void addDependencyRead(Instruction& instr,const VarDecl* d){
        for (auto& alias : aliasTable.getAliases(d))
        { 
            if(instr.dependencies.count(alias) == 0)
                instr.dependencies.insert({alias->getCanonicalDecl(),NodeDependency{true,false}});
            else
                instr.dependencies.find(alias->getCanonicalDecl())->second.isRead=true;  
        }
    }
    inline void addDependencyWrite(Instruction& instr,const VarDecl* d){
        for (auto& alias : aliasTable.getAliases(d))
        {
            if(instr.dependencies.count(alias) == 0)
                instr.dependencies.insert({alias->getCanonicalDecl(),NodeDependency{false,true}});
            else
                instr.dependencies.find(alias->getCanonicalDecl())->second.isWrite=true;  
        }
    }
    bool traverseSimpleElements(Stmt* s){
        if(isInHeaders(TheRewriter.getSourceMgr(),s->getBeginLoc())) 
            return true;
        
        Instruction instr{s,getStmtAsString(s,TheRewriter.getLangOpts()),false};
        handleStmt(*s,instr);
        functionsInstructionsVector.back().push_back(instr);
        return true;
    }
    void handleCXXOperatorCallExpr(const CXXOperatorCallExpr& ,Instruction&);
    void handleUnaryOperator(const UnaryOperator& ,Instruction&);
    void handleBinaryOperator(const BinaryOperator& ,Instruction&);
    void handleCallExpr(const CallExpr& ,Instruction&);
    void handleStmt(const Stmt& st,Instruction&);
    void handleMemberCallExpr(const CXXMemberCallExpr& ,Instruction&);
    void computeAliasesForRHS(const Expr* bop,std::unordered_set<const VarDecl*>&, Instruction& instr);
    Rewriter &TheRewriter;
    AliasTable aliasTable;
};

inline bool isInExceptionList(const ParmVarDecl& p){
    return p.getType().getAsString().find("std::shared_ptr") != std::string::npos;
}