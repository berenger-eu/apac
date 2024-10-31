/** This contains an implementation to convert
 * a sequential description of instructions to
 * a graph representation.
 * It is similar as the openmp task-dependency
 * approach.
 * The graph is represented as a list of nodes.
 * Each node contain a single instruction
 */
#pragma once
#include "AliasTable.hpp"
#include "Instruction.hpp"
#include "InstructionsOrderManager.hpp"
#include "Node.hpp"
#include "clang/AST/Decl.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Graph {
  int graphId;
  static int idCounter;
  std::vector<std::shared_ptr<Node>> nodes;
  std::vector<std::shared_ptr<Node>> roots;
  Graph() : graphId(idCounter++) {}
  void dump() const {
    llvm::errs() << "Graph: \n";
    for (const auto &node : nodes) {
      llvm::errs() << "Node: \n";
      node->dump();
    }
  }
  void fuseNodes(std::shared_ptr<Node> n1, std::shared_ptr<Node> n2) {
    n1->next = n2->next;
    for (auto &n : n2->next) {
      n.first->prev.erase(n2);
      n.first->prev.insert(n1);
    }
    for (auto &n2Dep : n2->dependencies) {
      if (n1->dependencies.count(n2Dep.first) == 0)
        n1->dependencies.insert(n2Dep);
      else
        n1->dependencies.at(n2Dep.first) =
            n1->dependencies.at(n2Dep.first) + n2Dep.second;
    }
    for (auto &n : n2->instructionPtr)
      n1->instructionPtr.push_back(n);
    for (auto &graph : n2->graph)
      n1->graph.push_back(graph);
    n1->instruction += "\n" + n2->instruction;
    this->nodes.erase(std::find(this->nodes.begin(), this->nodes.end(), n2));
  }
};
void updateInstructionOrderNode(const std::shared_ptr<Node> &, StmtOrder &);
void updateInstructionOrderFromGraph(const Graph &, StmtOrder &);

// Translate a list of instructions to a graph
Graph InstructionToGraph(
    const std::vector<Instruction> &, const AliasTable &, bool isLoop = false,
    std::shared_ptr<std::unordered_map<std::shared_ptr<aliasArg>,
                                       std::set<std::shared_ptr<Node>>>>
        previousDataUsedInRead = std::make_shared<std::unordered_map<
            std::shared_ptr<aliasArg>, std::set<std::shared_ptr<Node>>>>(),
    std::shared_ptr<
        std::unordered_map<std::shared_ptr<aliasArg>, std::shared_ptr<Node>>>
        previousDataUsedInWrite = std::make_shared<std::unordered_map<
            std::shared_ptr<aliasArg>, std::shared_ptr<Node>>>());

// Print the graph, mostly to debug
void PrintGraph(const Graph &);

// Generate a dot graph from a graph
void GenerateDotGraph(const std::vector<Graph> &graph,
                      const std::string &filename);

// Graph optimizations functions
void optimizeGraph(Graph &graph);

// Fuse nodes in the graph (nodes with a single link between them)
void nodesFusion(Graph &graph);
// Remove transitive dependencies in the graph
void transitiveReduction(Graph &graph);

// Generate all of the graph for a file, (generate one for each function)
std::vector<Graph> generateGraph(const std::vector<std::vector<Instruction>> &,
                                 StmtOrder &, const AliasTable &);
