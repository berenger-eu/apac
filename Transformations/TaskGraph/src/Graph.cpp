/** This contains an implementation to convert
 * a sequential description of instructions to
 * a graph representation.
 * It is similar as the openmp task-dependency
 * approach.
 * The graph is represented as a list of nodes.
 * Each node contain a single instruction
 */
#include "Graph.hpp"
#include <fstream>
#include <stack>

int Node::idCounter = 0;

Graph InstructionToGraph(const std::vector<Instruction> &inInstructions,
                         bool isLoop) {
  Graph graph;

  for (const auto &curInstruction : inInstructions) {
    auto node = std::make_shared<Node>();
    node->isLooped = isLoop;
    node->instruction =
        curInstruction.instructionString; // instruction.instruction;
    node->instructionPtr = std::vector<const Instruction *>();
    node->instructionPtr.push_back(&curInstruction);
    graph.nodes.push_back(node);
    if (curInstruction.complexInstruction) {
      if (isa<ForStmt>(curInstruction.instruction))
        node->graph.emplace_back(std::make_shared<Graph>(
            InstructionToGraph(curInstruction.scopedInstructions, true)));
      else
        node->graph.emplace_back(std::make_shared<Graph>(
            InstructionToGraph(curInstruction.scopedInstructions, isLoop)));
    }
    for (const auto &dep : curInstruction.dependencies) {
      node->dependencies.insert(dep);
    }
  }
  std::unordered_map<const clang::Decl *, std::set<std::shared_ptr<Node>>>
      dataUsedInRead;
  std::unordered_map<const clang::Decl *, std::shared_ptr<Node>>
      dataUsedInWrite;

  for (long unsigned int i = 0; i < inInstructions.size(); ++i) {
    auto node = graph.nodes[i];
    for (const auto &dep : inInstructions[i].dependencies) {
      bool readFound = false;
      if (dep.second.isRead) {
        if (dataUsedInWrite.find(dep.first) != dataUsedInWrite.end()) {
          if ((*dataUsedInWrite.find(dep.first)).second->id != node->id) {
            auto depNode = dataUsedInWrite[dep.first];
            depNode->addReadLink(depNode, node, dep.first);
          }
        }
        readFound = true;
      }
      if (dep.second.isWrite) {
        if (dataUsedInRead.find(dep.first) != dataUsedInRead.end()) {
          auto it = dataUsedInRead.find(dep.first);
          for (const auto &depNode : it->second) {
            if (depNode->id == node->id) {
              continue;
            }
            depNode->addWriteLink(depNode, node, dep.first);
          }

          dataUsedInRead.erase(dep.first);
        } else if (dataUsedInWrite.find(dep.first) != dataUsedInWrite.end()) {
          auto it = dataUsedInWrite.find(dep.first);
          if (it->second->id != node->id) {
            auto depNode = dataUsedInWrite[dep.first];
            depNode->addWriteLink(depNode, node, dep.first);
          }
        }
        dataUsedInWrite[dep.first] = node;
      }
      if (readFound)
        dataUsedInRead[dep.first].insert(node);
    }
  }

  for (const auto &node : graph.nodes)
    if (node->prev.empty())
      graph.roots.push_back(node);

  return graph;
}

void PrintGraph(const Graph &inGraph) {
  for (const auto &node : inGraph.nodes) {
    std::cout << "Node: " << node->id << " Instruction: " << node->instruction
              << std::endl;
    std::cout << "-- Next: ";
    for (const auto &next : node->next) {
      std::cout << next.first->id << " ";
    }
    std::cout << std::endl;
    std::cout << "-- Prev: ";
    for (const auto &prev : node->prev) {
      std::cout << prev->id << " ";
    }
    std::cout << std::endl;
    for (auto subGraph : node->graph) {
      std::cout << "-- SubGraph: " << std::endl;
      PrintGraph(*subGraph);
    }
  }
  std::cout << "Roots: " << std::endl;
  for (const auto &root : inGraph.roots)
    std::cout << root->id << " ";
  std::cout << std::endl;
}

void transitiveReduction(Graph &graph) {
  for (auto node : graph.nodes) {
    std::set<std::shared_ptr<Node>> visited;
    for (auto nextNode : node->next) {
      std::stack<std::shared_ptr<Node>> toVisit;
      for (auto nextNextNode : nextNode.first->next)
        toVisit.push(nextNextNode.first);
      while (!toVisit.empty()) {
        auto curNode = toVisit.top();
        toVisit.pop();
        if (visited.count(curNode))
          continue;
        visited.insert(curNode);

        for (auto nextNode : curNode->next) {
          toVisit.push(nextNode.first);
        }
      }
    }
    std::vector<std::shared_ptr<Node>> toRemove;
    for (auto nextNode : node->next)
      if (visited.count(nextNode.first))
        toRemove.push_back(nextNode.first);
    for (auto nextNode : toRemove) {
      node->next.erase(nextNode);
      nextNode->prev.erase(node);
    }
  }
}

void nodesFusion(Graph &graph) {
  std::stack<std::shared_ptr<Node>> toRemove;
  for (long unsigned int i = 0; i < graph.nodes.size(); i++) {
    auto node = graph.nodes[i];
    if (node->instructionPtr.size() == 1 &&
        node->instructionPtr[0]->complexInstruction)
      continue;
    while (
        node->next.size() == 1 && node->next.begin()->first->prev.size() == 1 &&
        !(node->next.begin()->first->instructionPtr.size() == 1 &&
          node->next.begin()->first->instructionPtr[0]->complexInstruction)) {
      toRemove.push(node->next.begin()->first);
      graph.fuseNodes(node, (node->next.begin()->first));
    }
    // TODO: handle multiple subgraphs (might happen after fusion of nodes)
    for (auto subGraph : node->graph)
      optimizeGraph(*subGraph);
  }
}
void nodesComputeInOut(Graph &graph) {
  for (auto &node : graph.nodes) {
    node->computeInOutInDep();
    for (auto &subGraph : node->graph)
      nodesComputeInOut(*subGraph);
  }
}
void optimizeGraph(Graph &graph) {
  transitiveReduction(graph);
  nodesFusion(graph);
  nodesComputeInOut(graph);
}

void updateInstructionOrderNode(
    const std::shared_ptr<Node> &node, StmtOrder &orderManager,
    std::unordered_set<std::shared_ptr<Node>> &visited) {
  if (visited.count(node))
    return;
  for (const auto &prev : node->prev)
    if (!visited.count(prev))
      updateInstructionOrderNode(prev, orderManager, visited);
  visited.insert(node);
  std::vector<const Stmt *> vectStmts;
  for (auto instr : node->instructionPtr)
    vectStmts.push_back(instr->instruction);
  fuseInstructions(vectStmts, orderManager);
  for (long unsigned int i = 0; i < node->graph.size(); i++) {
    auto subGraph = node->graph[i];
    long unsigned int count = -1;
    for (auto instr : node->instructionPtr)
      if (instr->complexInstruction && ++count == i)
        updateInstructionOrderFromGraph(
            *subGraph, *orderManager.getSubStmtOrder(instr->instruction));
  }
  orderManager.addNodeToGroup(node);
}

void updateInstructionOrderFromGraph(const Graph &graph,
                                     StmtOrder &orderManager) {
  std::unordered_set<std::shared_ptr<Node>> visited;
  for (const auto &node : graph.nodes)
    updateInstructionOrderNode(node, orderManager, visited);
  // Separated just in case
}

// Generate all of the graph for a file, (generate one for each function)
std::vector<Graph>
generateGraph(const std::vector<std::vector<Instruction>> &graphVector,
              StmtOrder &orderManager) {
  std::vector<Graph> graphs;
  for (auto &functionInstructions : graphVector)
    graphs.emplace_back(InstructionToGraph(functionInstructions));
  // GenerateDotGraph(graphs, "rawGraph.dot");
  // GenerateDotGraph(graphs, "optimizedGraph.dot");
  return graphs;
}