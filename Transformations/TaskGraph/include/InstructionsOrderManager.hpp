#pragma once
#include "Node.hpp"
#include "common.hpp"
#include "clang/AST/Stmt.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <deque>
#include <set>
#include <unordered_map>
// Key will be the instruction
// value will be the list of instructions that will be put in place of the
// instruction Each key in the map will correspond to a #pragma scope This
// structure will be used to easily fuse and swap instructions
using namespace clang;

struct StmtOrder;
struct SmallerBeginLocation {
  bool operator()(
      const std::pair<const Stmt *, std::shared_ptr<StmtOrder>> &a,
      const std::pair<const Stmt *, std::shared_ptr<StmtOrder>> &b) const {
    return a.first->getBeginLoc() <= b.first->getBeginLoc();
  }
};
struct StmtOrder {
  bool isLooped;
  int groupCounter = 0;
  std::map<const Stmt *, int> instructionLinks;
  std::map<int, std::set<std::pair<const Stmt *, std::shared_ptr<StmtOrder>>,
                         SmallerBeginLocation>>
      instructionGroups;
  std::map<int, std::shared_ptr<Node>> nodesGroup;
  StmtOrder() : isLooped(false) {}
  inline void addInstructionToManager(const Stmt *key) {
    instructionLinks.insert({key, groupCounter});
    std::set<std::pair<const Stmt *, std::shared_ptr<StmtOrder>>,
             SmallerBeginLocation>
        temp;
    instructionGroups.insert({groupCounter, temp});
    if (isa<ForStmt>(key)) {
      std::shared_ptr<StmtOrder> temp = std::make_shared<StmtOrder>();
      temp->isLooped = true;
      instructionGroups.at(groupCounter++).insert({{key, temp}});
    } else
      instructionGroups.at(groupCounter++)
          .insert({{key, std::shared_ptr<StmtOrder>()}});
  }
  inline void addNodeToGroup(const std::shared_ptr<Node> &node) {
    if (node == nullptr || node->instructionPtr.empty())
      return;
    auto st = node->instructionPtr.front()->instruction;
    if (instructionLinks.count(st) == 0)
      return;
    int groupNum = instructionLinks.at(st);
    if (nodesGroup.count(groupNum) == 0)
      nodesGroup.insert({groupNum, node});
  }
  inline std::shared_ptr<StmtOrder> getSubStmtOrder(const Stmt *key) const {
    if (instructionLinks.count(key) == 0)
      return nullptr;
    int temp = instructionLinks.at(key);
    if (instructionGroups.count(instructionLinks.at(key)) == 0)
      return nullptr;
    for (const auto &instr : instructionGroups.at(temp))
      if (instr.first == key)
        return instr.second;
    return nullptr;
  }
  inline std::shared_ptr<Node> getNode(const Stmt *key) const {
    if (instructionLinks.count(key) == 0 ||
        nodesGroup.count(instructionLinks.at(key)) == 0)
      return nullptr;
    return nodesGroup.at(instructionLinks.at(key));
  }

  void dump() const {
    llvm::errs() << "Dumping StmtOrder\n";
    for (const auto &instruction : instructionGroups) {
      llvm::errs() << "Group " << instruction.first << "\n";
      for (const auto &instr : instruction.second) {
        instr.first->dump();
        if (instr.second != nullptr) {
          instr.second->dump();
        }
      }
    }
  }
  void moveInstruction(const Stmt *key1, const Stmt *key2);
} typedef StmtOrder;
// typedef std::map<const Stmt*,std::deque<const Stmt*>> instructionsOrder;

void fuseInstructions(const std::vector<const Stmt *>,
                      StmtOrder &instructionsOrderManager);
void modifyFile(Rewriter &TheRewriter,
                const StmtOrder &instructionsOrderManager);

bool isPragmaValid(const StmtOrder &instructionsOrderManager,
                   const auto &instructionGroup);
std::string createPragmaTaskString(const StmtOrder &instructionsOrderManager,
                                   const auto &instructionGroup);