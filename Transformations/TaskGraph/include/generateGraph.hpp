/** This contains an implementation to convert
 * a sequential description of instructions to
 * a graph representation.
 * It is similar as the openmp task-dependency
 * approach.
 * The graph is represented as a list of nodes.
 * Each node contain a single instruction
 */
#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>
#include <functional>
#include "PotTaskGraphInterface.hpp"
#include "clang/AST/Decl.h"
#include <fstream>


struct Node {
    
    
    static int idCounter;
    int id;
    
    std::unordered_set<std::shared_ptr<Node>> next;
    std::unordered_set<std::shared_ptr<Node>> prev;
    std::shared_ptr<struct Graph> graph;
    std::string instruction;
};

struct Graph {
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Node>> roots;
};

Graph InstructionToGraph(const std::vector<Instruction>& );

void PrintGraph(const Graph& );

void GenerateDotGraph(const std::vector<Graph>& graph, const std::string& filename);

//Generate all of the graph for a file, (generate one for each function)
void generateGraph(const std::vector<std::vector<Instruction>>& );
