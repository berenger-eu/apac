#pragma once
#include <unordered_map>
#include <deque>
#include <set>
#include "clang/AST/Stmt.h"
#include "common.hpp"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"
//Key will be the instruction 
//value will be the list of instructions that will be put in place of the instruction
//Each key in the map will correspond to a #pragma scope
//This structure will be used to easily fuse and swap instructions
using namespace clang;

struct StmtOrder;
struct SmallerBeginLocation {
  bool operator()(const std::pair<const Stmt*,std::shared_ptr<StmtOrder>>& a, const std::pair<const Stmt*,std::shared_ptr<StmtOrder>>& b) const {
    return a.first->getBeginLoc() <= b.first->getBeginLoc();
  }

};
struct StmtOrder{
    int groupCounter = 0;
    std::map<const Stmt*,int> instructionLinks;
    std::map<int,std::set<std::pair<const Stmt*,std::shared_ptr<StmtOrder>>,SmallerBeginLocation>> instructionGroups;
    inline void addInstructionToManager(const Stmt* key)
    {
        instructionLinks.insert({key,groupCounter});
        std::set<std::pair<const Stmt*,std::shared_ptr<StmtOrder>>,SmallerBeginLocation> temp;
        instructionGroups.insert({groupCounter,temp});
        instructionGroups.at(groupCounter++).insert({{key,std::shared_ptr<StmtOrder>()}});
    }
    void dump(){
        for(const auto& instruction : instructionGroups)
        {
            llvm::errs()<<"Group "<<instruction.first<<"\n";
            for(const auto& instr : instruction.second)
            {
                instr.first->dump();
            }
        }
    }
    void moveInstruction(const Stmt* key1,const Stmt* key2);
} typedef StmtOrder;
// typedef std::map<const Stmt*,std::deque<const Stmt*>> instructionsOrder;

void fuseInstructions(const std::vector<const Stmt*>,StmtOrder& instructionsOrderManager);
void modifyFile(Rewriter& TheRewriter,const StmtOrder& instructionsOrderManager);