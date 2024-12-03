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
int Graph::idCounter = 0;
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

  // Create a node for each instruction
  for (const auto &curInstruction : inInstructions) {
    auto node = std::make_shared<Node>();
    node->graphId = graph.graphId;
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
  auto &dataUsedInRead = *dataUsedInReadPtr;
  auto &dataUsedInWrite = *dataUsedInWritePtr;

  // Iterate over all instructions
  for (long unsigned int i = 0; i < inInstructions.size(); ++i) {
    auto node = graph.nodes[i];
    // Iterate over all dependencies of the current instruction

    handleInstructionDeps(node, inInstructions[i].dependencies, dataUsedInRead,
                          dataUsedInWrite, aliasTable);

    // If the instruction has a sub scope, we create a new graph
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
void handleInstructionDeps(
    std::shared_ptr<Node> &node,
    const std::unordered_map<std::shared_ptr<aliasArg>, NodeDependency> &deps,
    std::unordered_map<std::shared_ptr<aliasArg>,
                       std::set<std::shared_ptr<Node>>> &dataUsedInRead,
    std::unordered_map<std::shared_ptr<aliasArg>, std::shared_ptr<Node>>
        &dataUsedInWrite,
    const AliasTable &aliasTable) {
  // Parent elements are read only (if considered write, it would ignore
  // current precision level) Example, a[1]=4;a[2]=1; if a is considered
  // write, then a[1] and a[2] can't be parallelized If considered read,
  // then a[1] and a[2] can be parallelized (and both elements are still
  // write) Children elements are considered write (if the element is write)
  std::vector<std::pair<std::shared_ptr<Node>, std::shared_ptr<aliasArg>>>
      dataReadBuffer;
  std::vector<std::pair<std::shared_ptr<Node>, std::shared_ptr<aliasArg>>>
      dataWriteBuffer;
  std::vector<std::shared_ptr<aliasArg>> dataReadEraseBuffer;
  for (auto &dep : deps) {

    auto &depElem = dep.first;
    for (const auto &depArrayElem :
         aliasTable.getArrayElementParents(dep.first)) {
      bool readFound = false;

      if (dep.second.isRead) {
        if (dataUsedInWrite.find(depArrayElem) != dataUsedInWrite.end()) {
          if ((*dataUsedInWrite.find(depArrayElem)).second->id != node->id) {
            auto depNode = dataUsedInWrite[depArrayElem];
            if (depElem->hasUnknownIndex)
              depNode->addReadLink(depNode, node, depElem);
            else
              depNode->addReadLink(depNode, node, depArrayElem);
          }
        }
        readFound = true;
      }
      if (readFound)
        dataReadBuffer.push_back({node, depArrayElem});
    }
    for (const auto &depArrayElem :
         aliasTable.getArrayElementChildren(dep.first)) {
      bool readFound = false;

      if (dep.second.isRead) {
        if (dataUsedInWrite.find(depArrayElem) != dataUsedInWrite.end()) {
          if ((*dataUsedInWrite.find(depArrayElem)).second->id != node->id) {
            auto depNode = dataUsedInWrite[depArrayElem];
            if (depElem->hasUnknownIndex)
              depNode->addReadLink(depNode, node, depElem);
            else
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
            if (depElem->hasUnknownIndex)
              depNode->addWriteLink(depNode, node, depElem);
            else
              depNode->addWriteLink(depNode, node, depArrayElem);
          }
          dataReadEraseBuffer.push_back(depArrayElem);
        } else if (dataUsedInWrite.find(depArrayElem) !=
                   dataUsedInWrite.end()) {
          auto it = dataUsedInWrite.find(depArrayElem);
          if (it->second->id != node->id) {
            auto depNode = dataUsedInWrite[depArrayElem];
            if (depElem->hasUnknownIndex)
              depNode->addWriteLink(depNode, node, depElem);
            else
              depNode->addWriteLink(depNode, node, depArrayElem);
          }
        }
        dataWriteBuffer.push_back({node, depArrayElem});
      }
      if (readFound)
        dataReadBuffer.push_back({node, depArrayElem});
    }
  }
  for (const auto &depArrayElem : dataReadEraseBuffer)
    dataUsedInRead.erase(depArrayElem);
  for (const auto &depArrayElem : dataReadBuffer)
    dataUsedInRead[depArrayElem.second].insert(depArrayElem.first);
  for (const auto &depArrayElem : dataWriteBuffer)
    dataUsedInWrite[depArrayElem.second] = depArrayElem.first;
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
        node->instructionPtr[0]->complexInstruction) {
      for (auto subGraph : node->graph) {
        optimizeGraph(*subGraph);
      }
      continue;
    }
    std::shared_ptr<Node> nextNode;
    if (node->next.size() == 1)
      nextNode = node->next.begin()->first;
    while (
        // Only one next node, and only one previous node for the next node
        node->next.size() == 1 && nextNode->prev.size() == 1 &&
        // The same graph (or subgraph), so same scope
        nextNode->graphId == node->graphId &&
        // The next node is not a complex instruction or a node that should not
        // be fused
        !((nextNode->instructionPtr.size() == 1 &&
           nextNode->instructionPtr[0]->complexInstruction) ||
          nextNode->instructionPtr[0]->noFusion) &&
        hasCommonArrayElement(node, nextNode)) {
      toRemove.push(node->next.begin()->first);
      graph.fuseNodes(node, (node->next.begin()->first));
      if (node->next.size() == 1)
        nextNode = node->next.begin()->first;
    }
    // TODO: handle multiple subgraphs (might happen after fusion of nodes)
  }
}
bool hasCommonArrayElement(std::shared_ptr<Node> &node1,
                           std::shared_ptr<Node> &node2) {
  auto nextAliasesMap = node1->next[node2];
  bool result = true;
  bool hasArrayUnknownElements = false;
  for (auto &nextNode : node1->next) {
    for (auto &nextLink : nextNode.second) {
      auto alias1 = nextLink.first;
      for (auto &nodeDep : node1->dependencies) {
        auto alias2 = nodeDep.first;
        if (indexesMatch(alias1->indexes, alias2->indexes)) {
          if (!hasArrayUnknownElements) {
            result = false;
            hasArrayUnknownElements = true;
          }
        }
        if (alias1 == alias2) {
          result = true;
        }
      }
    }
  }

  return result;
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

void updateInstructionOrderNode(const std::shared_ptr<Node> &node,
                                StmtOrder &orderManager) {
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
  for (const auto &node : graph.nodes)
    updateInstructionOrderNode(node, orderManager);
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