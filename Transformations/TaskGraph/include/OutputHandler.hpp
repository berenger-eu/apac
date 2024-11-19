#pragma once
#include "Graph.hpp"
#include "InstructionsOrderManager.hpp"
#include "clang/AST/Decl.h"
class OutputHandler {
public:
  OutputHandler(Rewriter &TheRewriter)
      : invisibleNodeCounter(0), TheRewriter(TheRewriter) {}
  void GenerateDotGraph(const std::vector<Graph> &graphs,
                        const std::string &filename);

  inline void modifyFile(const StmtOrder &instructionsOrderManager) {
    for (const auto &instructionGroup :
         instructionsOrderManager.instructionGroups) {
      modifyGroup(instructionsOrderManager, instructionGroup.second, true);
    }
  }

private:
  void subGenerateDotGraph(const Graph &graph, std::ofstream &file);
  void generateLinks(const Graph &inGraph, std::ofstream &file);
  std::string modifyGroup(const StmtOrder &instructionsOrderManager,
                          const InstructionGroup &instructionGroup,
                          bool writeInFile = false);
  std::string
  modifiedStringForInstruction(const StmtOrder &instructionsOrderManager,
                               const Stmt *instr);
  std::string
  modifiedStringdForInstructionIf(const StmtOrder &instructionsOrderManager,
                                  const Stmt *instr);
  bool isPragmaValid(const StmtOrder &instructionsOrderManager,
                     const InstructionGroup &instructionGroup) const;
  std::string createDependsString(const std::shared_ptr<Node> &node) const;
  void createPragmaString(const StmtOrder &instructionsOrderManager,
                          const InstructionGroup &instructionGroup,
                          std::string &pragmaStart, std::string &pragmaEnd);
  std::string
  createPragmaTaskString(const StmtOrder &instructionsOrderManager,
                         const InstructionGroup &instructionGroup) const;
  std::string
  createPragmaTaskWait(const StmtOrder &instructionsOrderManager,
                       const InstructionGroup &instructionGroup) const;

  int invisibleNodeCounter;
  Rewriter &TheRewriter;
};
