/** This contains an implementation to convert
 * a sequential description of instructions to
 * a graph representation.
 * It is similar as the openmp task-dependency
 * approach.
 * The graph is represented as a list of nodes.
 * Each node contain a single instruction
 */
#include "generateGraph.hpp"
#include <stack>
#include <fstream>

int Node::idCounter = 0;
void subGenerateDotGraph(const Graph& inGraph, std::ofstream& file){
    for(const auto& node : inGraph.nodes){
        file << "    " << node->id << " [label=\"" << node->instruction << "\"];\n";
        for (const auto& nextNode : node->next) {
            file << "    " << node->id << " -> " << nextNode->id << ";\n";
        }
        if(node->graph){
            file << "subgraph cluster_"<<node->id<<" {\n"<< "label = \"subGraph"<<node->id<<"\";\n";
            subGenerateDotGraph(*node->graph, file);
            file << "}\n";
        }
    }
}
void GenerateDotGraph(const std::vector<Graph>& graphs, const std::string& filename) {
    std::ofstream file(filename);
    for(auto graph : graphs)
    {
        file << "digraph G {\n";
        subGenerateDotGraph(graph, file);
        file << "}\n";
    }
    file.close();
}

Graph InstructionToGraph(const std::vector<Instruction>& inInstructions){
    Graph graph;

    for(const auto& curInstruction : inInstructions){
        auto node = std::make_shared<Node>();
        node->instruction = curInstruction.instructionString;//instruction.instruction;
        node->id = Node::idCounter++;
        graph.nodes.push_back(node);
        if(curInstruction.complexInstruction){
            node->graph = std::make_shared<Graph>();
            *node->graph = InstructionToGraph(curInstruction.scopedInstructions);
        }
    }
    std::unordered_map<const clang::Decl*, std::set<std::shared_ptr<Node>>> dataUsedInRead;
    std::unordered_map<const clang::Decl*, std::shared_ptr<Node>> dataUsedInWrite;

    for (int i = 0; i < inInstructions.size(); ++i){
        auto node = graph.nodes[i];
        std::cout<< "Node: "<<node->id<<std::endl;
        std::cout << "Instruction: " << inInstructions[i].instructionString << std::endl;
        std::cout<< "Dependencies: "<<inInstructions[i].dependencies.size()<<std::endl;
        std::cout<< "Read: "<<dataUsedInRead.size()<<std::endl;
        std::cout<< "Write: "<<dataUsedInWrite.size()<<std::endl;
        /*
        for (const auto& dep : inInstructions[i].dependencies){
            std::cout << "Dep: " << dep.second << std::endl;
        }
        for (const auto& red: dataUsedInRead){
            std::cout << "Read: " << red.first << std::endl;
        }
        for (const auto& red: dataUsedInWrite){
            std::cout << "Write: " << red.first<<" "<<red.second->id << std::endl;
        }*/
        for (const auto& dep : inInstructions[i].dependencies){
            if (dep.first == Access::READ){
                 if(dataUsedInWrite.find(dep.second) != dataUsedInWrite.end()){
                      if((*dataUsedInWrite.find(dep.second)).second->id!=node->id){
                        auto depNode = dataUsedInWrite[dep.second];
                        depNode->next.insert(node);
                        node->prev.insert(depNode);
                    }
                }
                dataUsedInRead[dep.second].insert(node);
            } 
            else {
                std::cout <<"Coucou1"<<std::endl;
                if( dataUsedInRead.find(dep.second) != dataUsedInRead.end()){
                    std::cout <<"Coucou2"<<std::endl;
                    auto it = dataUsedInRead.find(dep.second);
                    std::cout<< "Size: "<<it->first<<it->second.size()<<std::endl;
                    for (const auto& depNode : it->second){
                        if(depNode->id == node->id){
                            continue;
                        }
                        depNode->next.insert(node);
                        node->prev.insert(depNode);
                    }

                    dataUsedInRead.erase(dep.second);
                }
                else if(dataUsedInWrite.find(dep.second) != dataUsedInWrite.end()){
                    std::cout <<"Coucou3"<<std::endl;
                    auto it= dataUsedInWrite.find(dep.second);
                    if(it->second->id!=node->id){
                        auto depNode = dataUsedInWrite[dep.second];
                        depNode->next.insert(node);
                        node->prev.insert(depNode);
                    }    
                }
                dataUsedInWrite[dep.second] = node;            
            }
        }
    }

    for (const auto& node : graph.nodes){
        if (node->prev.empty()){
            graph.roots.push_back(node);
        }
    }

    return graph;
}

void PrintGraph(const Graph& inGraph){
    for(const auto& node : inGraph.nodes){
        std::cout << "Node: " << node->id << " Instruction: " << node->instruction << std::endl;
        std::cout << "-- Next: ";
        for (const auto& next : node->next){
            std::cout << next->id << " ";
        }
        std::cout << std::endl;
        std::cout << "-- Prev: ";
        for (const auto& prev : node->prev){
            std::cout << prev->id << " ";
        }
        std::cout << std::endl;
        if(node->graph){
            std::cout << "-- SubGraph: " << std::endl;
            PrintGraph(*node->graph);
        }

    }
    std::cout << "Roots: " << std::endl;
    for(const auto& root : inGraph.roots)
        std::cout << root->id <<" ";
    std::cout << std::endl;
}

//Generate all of the graph for a file, (generate one for each function)
void generateGraph(const std::vector<std::vector<Instruction>>& graphVector){
    std::vector<Graph> graphs;
    for (auto& functionInstructions : graphVector)
    {
        
        auto graph = InstructionToGraph(functionInstructions);
        PrintGraph(graph);
        graphs.push_back(graph);
    }
    GenerateDotGraph(graphs, "graph.dot");
}