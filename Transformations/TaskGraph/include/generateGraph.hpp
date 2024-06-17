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
#include "AliasTable.hpp"
#include "clang/AST/Decl.h"
#include <fstream>

struct NodeDependency {
    bool isRead;
    bool isWrite;
};




struct Node {
    
    
    static int idCounter;
    int id;
    std::unordered_map<std::shared_ptr<Node>,std::unordered_map<const NamedDecl*,NodeDependency>> next;
    std::unordered_set<std::shared_ptr<Node>> prev;
    std::shared_ptr<struct Graph> graph;
    std::string instruction;
    void addLink(std::shared_ptr<Node> n, bool isRead, bool isWrite, const NamedDecl* arg){
        if(this->next.count(n) == 0){
            this->next.insert({n,std::unordered_map<const NamedDecl*,NodeDependency>()}); 
            this->next.at(n).insert({arg,{isRead,isWrite}});
            n->prev.insert(std::make_shared<Node>(*this));
        }       
        else{
            this->next.at(n).find(arg)->second.isRead = this->next.at(n).find(arg)->second.isRead || isRead;
            this->next.at(n).find(arg)->second.isWrite = this->next.at(n).find(arg)->second.isWrite || isWrite;
            // this->next.at(n).find(arg).isRead = this->next.at(n).find(arg).isRead || isRead;
            // this->next.at(n).second.find(arg).isWrite = this->next.at(n).find(arg).isWrite || isWrite;
        }
        // this->next.at(n).insert({isRead,isWrite,arg});
        // n->prev.insert(shared_from_this());
    }
    inline void addReadLink(std::shared_ptr<Node> n, const NamedDecl* arg){
        addLink(n,true,false,arg);
    }
    inline void addWriteLink(std::shared_ptr<Node> n, const NamedDecl* arg){
        addLink(n,false,true,arg);
    }
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
