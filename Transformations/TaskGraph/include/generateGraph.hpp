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
#include "InstructionsOrderManager.hpp"
#include "PotTaskGraphInterface.hpp"
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

struct Node {

  static int idCounter;
  int id;
  std::unordered_map<std::shared_ptr<Node>,
                     std::unordered_map<const NamedDecl *, NodeDependency>>
      next;
  std::unordered_set<std::shared_ptr<Node>> prev;
  std::vector<std::shared_ptr<struct Graph>> graph;
  std::string instruction;
  std::vector<const Instruction *> instructionPtr;
  void dump() {
    llvm::errs() << "Instructions: " << instruction << "\n";
    for (auto &instr : instructionPtr) {

      instr->dump();
    }
    llvm::errs() << "\n";
  }
  void addLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n, bool isRead,
               bool isWrite, const NamedDecl *arg) {
    if (arg == nullptr)
      return;
    if (this->next.count(n) == 0)
      this->next.insert(
          {n, std::unordered_map<const NamedDecl *, NodeDependency>()});
    if (this->next.at(n).count(arg) == 0) {
      this->next.at(n).insert({arg, {isRead, isWrite}});
      n->prev.insert(curN);
    } else {
      this->next.at(n).at(arg).isRead =
          this->next.at(n).at(arg).isRead || isRead;
      this->next.at(n).at(arg).isWrite =
          this->next.at(n).at(arg).isWrite || isWrite;
    }
  }
  inline void addReadLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                          const NamedDecl *arg) {
    addLink(curN, n, true, false, arg);
  }
  inline void addWriteLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                           const NamedDecl *arg) {
    addLink(curN, n, false, true, arg);
  }
};

struct Graph {
  std::vector<std::shared_ptr<Node>> nodes;
  std::vector<std::shared_ptr<Node>> roots;
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
    for (auto &n : n2->instructionPtr)
      n1->instructionPtr.push_back(n);
    for (auto &graph : n2->graph)
      n1->graph.push_back(graph);
    n1->instruction += "\n" + n2->instruction;
    this->nodes.erase(std::find(this->nodes.begin(), this->nodes.end(), n2));
  }
};
void updateInstructionOrderNode(const std::shared_ptr<Node> &, StmtOrder &,
                                std::unordered_set<std::shared_ptr<Node>> &);
void updateInstructionOrderFromGraph(const Graph &, StmtOrder &);

// Translate a list of instructions to a graph
Graph InstructionToGraph(const std::vector<Instruction> &);

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
void generateGraph(const std::vector<std::vector<Instruction>> &, StmtOrder &);
