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

Graph InstructionToGraph(
    const std::vector<Instruction> &inInstructions,
    const AliasTable &aliasTable, bool isLoop,
    std::shared_ptr<std::unordered_map<std::shared_ptr<aliasArg>,
                                       std::set<std::shared_ptr<Node>>>>
        previousDataUsedInRead,
    std::shared_ptr<
        std::unordered_map<std::shared_ptr<aliasArg>, std::shared_ptr<Node>>>
        previousDataUsedInWrite) {
  Graph graph;

  for (const auto &curInstruction : inInstructions) {
    auto node = std::make_shared<Node>();
    node->isLooped = isLoop;
    node->instruction =
        curInstruction.instructionString; // instruction.instruction;
    node->instructionPtr = std::vector<const Instruction *>();
    node->instructionPtr.push_back(&curInstruction);
    graph.nodes.push_back(node);

    for (const auto &dep : curInstruction.dependencies) {
      node->dependencies.insert(dep);
    }
  }
  std::shared_ptr<std::unordered_map<std::shared_ptr<aliasArg>,
                                     std::set<std::shared_ptr<Node>>>>
      dataUsedInReadPtr;
  std::shared_ptr<
      std::unordered_map<std::shared_ptr<aliasArg>, std::shared_ptr<Node>>>
      dataUsedInWritePtr;
  if (previousDataUsedInRead != nullptr)
    dataUsedInReadPtr = previousDataUsedInRead;
  if (previousDataUsedInWrite != nullptr)
    dataUsedInWritePtr = previousDataUsedInWrite;
  std::unordered_map<std::shared_ptr<aliasArg>, std::set<std::shared_ptr<Node>>>
      &dataUsedInRead = *dataUsedInReadPtr;
  std::unordered_map<std::shared_ptr<aliasArg>, std::shared_ptr<Node>>
      &dataUsedInWrite = *dataUsedInWritePtr;

  // Iterate over all instructions
  for (long unsigned int i = 0; i < inInstructions.size(); ++i) {
    auto node = graph.nodes[i];
    // Iterate over all dependencies of the current instruction
    for (const auto &dep : inInstructions[i].dependencies) {
      // Parent elements are read only (if considered write, it would ignore
      // current precision level) Example, a[1]=4;a[2]=1; if a is considered
      // write, then a[1] and a[2] can't be parallelized If considered read,
      // then a[1] and a[2] can be parallelized (and both elements are still
      // write) Children elements are considered write (if the element is write)
      auto &depElem = dep.first;
      for (const auto &depArrayElem :
           aliasTable.getArrayElementParents(dep.first)) {
        bool readFound = false;

        if (dep.second.isRead) {
          if (dataUsedInWrite.find(depArrayElem) != dataUsedInWrite.end()) {
            if ((*dataUsedInWrite.find(depArrayElem)).second->id != node->id) {
              auto depNode = dataUsedInWrite[depArrayElem];
              depNode->addReadLink(depNode, node, depArrayElem);
            }
          }
          readFound = true;
        }
        if (readFound)
          dataUsedInRead[depArrayElem].insert(node);
      }
      for (const auto &depArrayElem :
           aliasTable.getArrayElementChildren(dep.first)) {
        bool readFound = false;

        if (dep.second.isRead) {
          if (dataUsedInWrite.find(depArrayElem) != dataUsedInWrite.end()) {
            if ((*dataUsedInWrite.find(depArrayElem)).second->id != node->id) {
              auto depNode = dataUsedInWrite[depArrayElem];
              depNode->addReadLink(depNode, node, depArrayElem);
            }
          }
          readFound = true;
        }
        if (dep.second.isWrite) {
          if (dataUsedInRead.find(depArrayElem) != dataUsedInRead.end()) {
            auto it = dataUsedInRead.find(depArrayElem);
            for (const auto &depNode : it->second) {
              if (depNode->id == node->id) {
                continue;
              }
              depNode->addWriteLink(depNode, node, depArrayElem);
            }

            dataUsedInRead.erase(depArrayElem);
          } else if (dataUsedInWrite.find(depArrayElem) !=
                     dataUsedInWrite.end()) {
            auto it = dataUsedInWrite.find(depArrayElem);
            if (it->second->id != node->id) {
              auto depNode = dataUsedInWrite[depArrayElem];
              depNode->addWriteLink(depNode, node, depArrayElem);
            }
          }
          dataUsedInWrite[depArrayElem] = node;
        }
        if (readFound)
          dataUsedInRead[depArrayElem].insert(node);
      }
    }

    if (inInstructions[i].complexInstruction) {
      if (isa<ForStmt>(inInstructions[i].instruction))
        node->graph.emplace_back(std::make_shared<Graph>(InstructionToGraph(
            inInstructions[i].scopedInstructions, aliasTable, true)));
      else {
        node->graph.emplace_back(std::make_shared<Graph>(
            InstructionToGraph(inInstructions[i].scopedInstructions, aliasTable,
                               isLoop, dataUsedInReadPtr, dataUsedInWritePtr)));
      }
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
    assert(subGraph != nullptr);
    long unsigned int count = -1;
    for (auto instr : node->instructionPtr)
      if (instr->complexInstruction && ++count == i) {
        auto subOrder = orderManager.getSubStmtOrder(instr->instruction);
        assert(subOrder != nullptr);
        updateInstructionOrderFromGraph(*subGraph, *subOrder);
      }
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
              StmtOrder &orderManager, const AliasTable &aliasTable) {
  std::vector<Graph> graphs;
  for (auto &functionInstructions : graphVector)
    graphs.emplace_back(InstructionToGraph(functionInstructions, aliasTable));
  // GenerateDotGraph(graphs, "rawGraph.dot");
  // GenerateDotGraph(graphs, "optimizedGraph.dot");
  return graphs;
}