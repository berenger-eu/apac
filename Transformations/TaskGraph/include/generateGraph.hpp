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
#include <set>
#include <memory>
#include <functional>
#include "PotTaskGraphInterface.hpp"

struct Node {
    int id;
    std::vector<std::shared_ptr<Node>> next;
    std::vector<std::shared_ptr<Node>> prev;
    std::string instruction;
};

struct Graph {
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Node>> roots;
};

auto InstructionToGraph(const std::vector<Instruction>& );

void PrintGraph(const Graph& );


//Generate all of the graph for a file, (generate one for each function)
void generateGraph(const std::vector<std::vector<Instruction>>& );
