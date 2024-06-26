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

struct SmallerBeginLocation {
  bool operator()(const Stmt*& a, const Stmt*& b) const {
    return a->getBeginLoc() <= b->getBeginLoc();
  }
  bool operator()(const Stmt* const & a, const Stmt* const & b) const {
    return a->getBeginLoc() <= b->getBeginLoc();
  }
};

struct StmtOrder{
    int groupCounter = 0;
    std::map<const Stmt*,int> instructionLinks;
    std::map<int,std::set<const Stmt*,SmallerBeginLocation>> instructionGroups;
    inline void addInstructionToManager(const Stmt* key)
    {
        instructionLinks.insert({key,groupCounter});
        instructionGroups.insert({groupCounter,std::set<const Stmt*,SmallerBeginLocation>()});
        instructionGroups.at(groupCounter++).insert(key);
    }
    void moveInstruction(const Stmt* key1,const Stmt* key2);
} typedef StmtOrder;
// typedef std::map<const Stmt*,std::deque<const Stmt*>> instructionsOrder;

void fuseInstructions(const std::vector<const Stmt*>,StmtOrder& instructionsOrderManager);
void modifyFile(Rewriter& TheRewriter,const StmtOrder& instructionsOrderManager);