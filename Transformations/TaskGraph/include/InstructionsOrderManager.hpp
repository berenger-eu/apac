#pragma once
#include <unordered_map>
#include <deque>
#include "clang/AST/Stmt.h"
#include "common.hpp"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"
//Key will be the instruction 
//value will be the list of instructions that will be put in place of the instruction
//Each key in the map will correspond to a #pragma scope
//This structure will be used to easily fuse and swap instructions
using namespace clang;
typedef std::map<const Stmt*,std::deque<const Stmt*>> instructionsOrder;

inline void addInstructionToManager(const Stmt* key,instructionsOrder& instructionsOrderManager)
{
    std::deque<const Stmt*> queue;
    queue.push_back(key);
    instructionsOrderManager.insert({key,queue});
}
void moveInstruction(const Stmt* key1,const Stmt* key2,instructionsOrder& instructionsOrderManager);
void fuseInstructions(const std::vector<const Stmt*>,instructionsOrder& instructionsOrderManager);
void modifyFile(Rewriter& TheRewriter,const instructionsOrder& instructionsOrderManager);