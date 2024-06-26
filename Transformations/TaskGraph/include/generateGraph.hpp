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
#include "InstructionsOrderManager.hpp"
#include "clang/AST/Decl.h"
#include <fstream>






struct Node {
    
    
    static int idCounter;
    int id;
    std::unordered_map<std::shared_ptr<Node>,std::unordered_map<const NamedDecl*,NodeDependency>> next;
    std::unordered_set<std::shared_ptr<Node>> prev;
    std::shared_ptr<struct Graph> graph;
    std::string instruction;
    std::vector<const Instruction*> instructionPtr;
    void dump()
    {
        llvm::errs()<<"Instructions: "<<instruction<<"\n";
        for(auto& instr:instructionPtr){

            instr->dump();
            
        }
        llvm::errs()<<"\n";
    }
    void addLink(std::shared_ptr<Node>curN,std::shared_ptr<Node> n, bool isRead, bool isWrite, const NamedDecl* arg){
        if(arg == nullptr)
            return;   
        if(this->next.count(n) == 0)
            this->next.insert({n,std::unordered_map<const NamedDecl*,NodeDependency>()});       
        if (this->next.at(n).count(arg) == 0){
            this->next.at(n).insert({arg,{isRead,isWrite}});
            n->prev.insert(curN);
        }
        else{
            this->next.at(n).at(arg).isRead = this->next.at(n).at(arg).isRead || isRead;
            this->next.at(n).at(arg).isWrite = this->next.at(n).at(arg).isWrite || isWrite;
        }
    }
    inline void addReadLink(std::shared_ptr<Node>curN,std::shared_ptr<Node> n, const NamedDecl* arg){
        addLink(curN,n,true,false,arg);
    }
    inline void addWriteLink(std::shared_ptr<Node>curN,std::shared_ptr<Node> n, const NamedDecl* arg){
        addLink(curN,n,false,true,arg);
    }
};



struct Graph {
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Node>> roots;
    void dump()
    {
        llvm::errs()<<"Graph: \n";
        for (const auto& node : nodes){
            llvm::errs()<<"Node: \n";
            node->dump();
        }
    }
    void fuseNodes(std::shared_ptr<Node> n1, std::shared_ptr<Node> n2){
        n1->next=n2->next;
        for (auto &n : n2->next){
            n.first->prev.erase(n2);
            n.first->prev.insert(n1);
        }
        for(auto& n : n2->instructionPtr)
            n1->instructionPtr.push_back(n);
        n1->instruction+="\n"+n2->instruction;
        this->nodes.erase(std::find(this->nodes.begin(),this->nodes.end(),n2)); 
    }
};
void updateInstructionOrderNode(const std::shared_ptr<Node>& ,StmtOrder& ,std::unordered_set<std::shared_ptr<Node>>& );
void updateInstructionOrderFromGraph(const Graph& ,StmtOrder& );

Graph InstructionToGraph(const std::vector<Instruction>& );

void PrintGraph(const Graph& );

void GenerateDotGraph(const std::vector<Graph>& graph, const std::string& filename);
void optimizeGraph(Graph& graph);

//Generate all of the graph for a file, (generate one for each function)
void generateGraph(const std::vector<std::vector<Instruction>>& ,StmtOrder& );
