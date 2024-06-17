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
int invisibleNodeCounter = 1;

void subGenerateDotGraph(const Graph& inGraph, std::ofstream& file){
    file << "    invisibleNodeScope_" << invisibleNodeCounter << " [style=invis];\n";
    for(const auto& node : inGraph.nodes){
        file << "    " << node->id << " [label=\"" << node->instruction << "\"];\n";
        for (const auto& nextNode : node->next) {
            std::stringstream ss;
            for(auto iterBegin = nextNode.second.begin(); iterBegin != nextNode.second.end(); iterBegin++){
                if(iterBegin != nextNode.second.begin())
                    ss << ",";
                ss << iterBegin->first->getNameAsString()<< (iterBegin->second.isRead?"R":"")<<(iterBegin->second.isWrite?"W":"");
            }
            file << "    " << node->id << " -> " << nextNode.first->id << "[label=\"  "<<ss.str()<<"\"];\n";
        }
        if(node->graph){
            file << "    " << node->id << " -> invisibleNodeScope_" << ++invisibleNodeCounter << "[label=\"innerScope\"];\n";
            file << "subgraph cluster_"<<node->id<<" {\n"<< "label = \"subGraph"<<node->id<<"\";\n";
            subGenerateDotGraph(*node->graph, file);
            file << "}\n";
        }
    }
}
void GenerateDotGraph(const std::vector<Graph>& graphs, const std::string& filename) {
    std::ofstream file(filename);
    file << "digraph G {\n";
    int i = 0;
    for(auto graph : graphs)
    {
        file << "subgraph cluster_f"<<i <<" {\n";
        subGenerateDotGraph(graph, file);
        file << "}\n";
        i++;
    }
    file << "}\n";
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

    for (long unsigned int i = 0; i < inInstructions.size(); ++i){
        auto node = graph.nodes[i];
        llvm::errs()<<"Instruction: "<<node->instruction<<"\n";
        for (const auto& dep : inInstructions[i].dependencies){
            bool readFound = false;
            if (dep.second.isRead){
                 if(dataUsedInWrite.find(dep.first) != dataUsedInWrite.end()){
                      if((*dataUsedInWrite.find(dep.first)).second->id!=node->id){
                        auto depNode = dataUsedInWrite[dep.first];
                        depNode->addReadLink(node,dep.first);
                        llvm::errs()<<"Adding read link from "<<depNode->instruction<<" to "<<node->instruction<<"\n";
                        /*
                        if(depNode->next.count(node)==0)
                            depNode->next.insert({node,std::unordered_set<NodeDependency>()});
                        depNode->next.at(node).insert(N);
                        node->prev.insert(depNode);
                        */
                    }
                }
                readFound = true;
                
            } 
            if (dep.second.isWrite){
                if( dataUsedInRead.find(dep.first) != dataUsedInRead.end()){
                    auto it = dataUsedInRead.find(dep.first);
                    for (const auto& depNode : it->second){
                        if(depNode->id == node->id){
                            continue;
                        }
                        depNode->addWriteLink(node,dep.first);
                        llvm::errs()<<"1Adding write link from "<<depNode->instruction<<" to "<<node->instruction<<"\n";
                        /*
                        if(depNode->next.count(node)==0)
                            depNode->next.insert({node,std::unordered_set<std::string>()});
                        depNode->next.at(node).insert(dep.second->getNameAsString());
                        node->prev.insert(depNode);
                        */
                    }

                    dataUsedInRead.erase(dep.first);
                }
                else if(dataUsedInWrite.find(dep.first) != dataUsedInWrite.end()){
                    auto it= dataUsedInWrite.find(dep.first);
                    if(it->second->id!=node->id){
                        auto depNode = dataUsedInWrite[dep.first];
                        depNode->addWriteLink(node,dep.first);
                        llvm::errs()<<"2Adding write link from "<<depNode->instruction<<" to "<<node->instruction<<"\n";
                        /*  
                        if(depNode->next.count(node)==0)
                            depNode->next.insert({node,std::unordered_set<std::string>()});
                        depNode->next.at(node).insert(dep.second->getNameAsString());
                        node->prev.insert(depNode);
                        */
                    }    
                }
                if(readFound)
                    dataUsedInRead[dep.first].insert(node);
                dataUsedInWrite[dep.first] = node;            
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
            std::cout << next.first->id << " ";
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
        //PrintGraph(graph);
        graphs.push_back(graph);
    }
    GenerateDotGraph(graphs, "graph.dot");
}