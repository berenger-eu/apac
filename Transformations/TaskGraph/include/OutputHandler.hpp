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

  void modifyFile(const StmtOrder &instructionsOrderManager);

private:
  void subGenerateDotGraph(const Graph &graph, std::ofstream &file);
  void generateLinks(const Graph &inGraph, std::ofstream &file);
  std::string
  modifiedStringForInstruction(const StmtOrder &instructionsOrderManager,
                               const Stmt *instr);

  bool isPragmaValid(const StmtOrder &instructionsOrderManager,
                     const int &instructionGroupNum) const;
  std::string createDependsString(const std::shared_ptr<Node> &node) const;
  std::string createPragmaTaskString(const StmtOrder &instructionsOrderManager,
                                     const int &instructionGroupNum) const;
  std::string createPragmaTaskWait(const StmtOrder &instructionsOrderManager,
                                   const int &instructionGroupNum) const;

  int invisibleNodeCounter;
  Rewriter &TheRewriter;
};
