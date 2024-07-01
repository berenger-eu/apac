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
  std::shared_ptr<struct Graph> graph;
  std::string instruction;
  const Instruction *instructionPtr;
  // Adds a link between the current node and the node n, link being a read
  // and/or write of the argument arg
  void addLink(std::shared_ptr<Node> n, bool isRead, bool isWrite,
               const NamedDecl *arg) {
    if (arg == nullptr) {
      return;
    }
    if (this->next.count(n) == 0) {
      this->next.insert(
          {n, std::unordered_map<const NamedDecl *, NodeDependency>()});
    }
    if (this->next.at(n).count(arg) == 0) {
      this->next.at(n).insert({arg, {isRead, isWrite}});
      n->prev.insert(std::make_shared<Node>(*this));
    } else {
      this->next.at(n).at(arg).isRead =
          this->next.at(n).at(arg).isRead || isRead;
      this->next.at(n).at(arg).isWrite =
          this->next.at(n).at(arg).isWrite || isWrite;
    }
  }
  // Add a read link to the node n, because the argument arg is read
  inline void addReadLink(std::shared_ptr<Node> n, const NamedDecl *arg) {
    addLink(n, true, false, arg);
  }
  // Add a write link to the node n, because the argument arg is written
  inline void addWriteLink(std::shared_ptr<Node> n, const NamedDecl *arg) {
    addLink(n, false, true, arg);
  }
};

struct Graph {
  std::vector<std::shared_ptr<Node>> nodes;
  std::vector<std::shared_ptr<Node>> roots;
};

// Translate a list of instructions to a graph
Graph InstructionToGraph(const std::vector<Instruction> &);

// Print the graph, mostly to debug
void PrintGraph(const Graph &);

// Generate a dot graph from a graph
void GenerateDotGraph(const std::vector<Graph> &graph,
                      const std::string &filename);

// Generate all of the graphs object for a file, (generate one for each
// function)
void generateGraph(const std::vector<std::vector<Instruction>> &);
