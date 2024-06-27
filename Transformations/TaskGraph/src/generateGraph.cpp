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
int invisibleNodeCounter = 0;

void subGenerateDotGraph(const Graph& inGraph, std::ofstream& file){
    file << "    invisibleNodeScope_" << invisibleNodeCounter++ << " [style=invis];\n";
    for(const auto& node : inGraph.nodes){
        file << "    " << node->id << " [label=\"" << node->instruction; 
        for(auto instr :node->instructionPtr)
            for(auto alias : instr->curAliases){
            file << "\n" ;
            for(int nbStar = 0; nbStar < getPtrDepthAccess(alias.first->getType(),alias.second->getType(),alias.first->getASTContext()); nbStar++)
                file << "*";
            file<<alias.first->getNameAsString() << " : " << alias.second->getNameAsString();
        }
        file<<"\"];\n";
        for (const auto& nextNode : node->next) {
            std::stringstream ss;
            for(auto iterBegin = nextNode.second.begin(); iterBegin != nextNode.second.end(); iterBegin++){
                if(iterBegin != nextNode.second.begin())
                    ss << ",";
                ss << iterBegin->first->getNameAsString()<< (iterBegin->second.isRead?"R":"")<<(iterBegin->second.isWrite?"W":"");
            }
            file << "    " << node->id << " -> " << nextNode.first->id << "[label=\"  "<<ss.str()<<"\"];\n";
        }
        for(auto& subGraph : node->graph){
            file << "    " << node->id << " -> invisibleNodeScope_" << invisibleNodeCounter << "[label=\"innerScope\"];\n";
            file << "subgraph cluster_"<<node->id<<" {\n"<< "label = \"subGraph"<<node->id<<"\";\n";
            subGenerateDotGraph(*subGraph, file);
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
        node->instructionPtr = std::vector<const Instruction*>();
        node->instructionPtr.push_back(&curInstruction);
        node->id = Node::idCounter++;
        graph.nodes.push_back(node);
        if(curInstruction.complexInstruction){
            node->graph.emplace_back(std::make_shared<Graph>(InstructionToGraph(curInstruction.scopedInstructions)));
        }
    }
    std::unordered_map<const clang::Decl*, std::set<std::shared_ptr<Node>>> dataUsedInRead;
    std::unordered_map<const clang::Decl*, std::shared_ptr<Node>> dataUsedInWrite;

    for (long unsigned int i = 0; i < inInstructions.size(); ++i){
        auto node = graph.nodes[i];
        for (const auto& dep : inInstructions[i].dependencies){
            bool readFound = false;
            if (dep.second.isRead){
                 if(dataUsedInWrite.find(dep.first) != dataUsedInWrite.end()){
                      if((*dataUsedInWrite.find(dep.first)).second->id!=node->id){
                        auto depNode = dataUsedInWrite[dep.first];
                        depNode->addReadLink(depNode,node,dep.first);
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
                        depNode->addWriteLink(depNode,node,dep.first);
                    }

                    dataUsedInRead.erase(dep.first);
                }
                else if(dataUsedInWrite.find(dep.first) != dataUsedInWrite.end()){
                    auto it= dataUsedInWrite.find(dep.first);
                    if(it->second->id!=node->id){
                        auto depNode = dataUsedInWrite[dep.first];
                        depNode->addWriteLink(depNode,node,dep.first);
                    }    
                }       
                dataUsedInWrite[dep.first] = node;            
            }
            if(readFound)
                dataUsedInRead[dep.first].insert(node);
            
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
        for(auto subGraph : node->graph){
            std::cout << "-- SubGraph: " << std::endl;
            PrintGraph(*subGraph);
        }

    }
    std::cout << "Roots: " << std::endl;
    for(const auto& root : inGraph.roots)
        std::cout << root->id <<" ";
    std::cout << std::endl;
}
void optimizeGraph(Graph& graph)
{
    std::stack<std::shared_ptr<Node>> toRemove;
    for(long unsigned int i=0;i<graph.nodes.size();i++)     
    {
        auto node = graph.nodes[i];
        while(node->next.size() == 1 && node->next.begin()->first->prev.size() == 1)
        {
            toRemove.push(node->next.begin()->first);
            graph.fuseNodes(node,(node->next.begin()->first));
        }  
        //TODO: handle multiple subgraphs (might happen after fusion of nodes)
        for(auto subGraph : node->graph)
             optimizeGraph(*subGraph);
    }
}

void updateInstructionOrderNode(const std::shared_ptr<Node>& node,
StmtOrder& orderManager,std::unordered_set<std::shared_ptr<Node>>& visited)
{
    if(visited.count(node))
        return;
    for(const auto& prev: node->prev)
        if(!visited.count(prev))
            updateInstructionOrderNode(prev,orderManager,visited);
    visited.insert(node);
    std::vector<const Stmt*> vectStmts;
    for(auto instr : node->instructionPtr)
        vectStmts.push_back(instr->instruction);
    fuseInstructions(vectStmts,orderManager);
    /*
    if(node->graph)
        updateInstructionOrderFromGraph(*node->graph,orderManager);
    */
}

void updateInstructionOrderFromGraph(const Graph& graph,StmtOrder& orderManager){
    std::unordered_set<std::shared_ptr<Node>> visited;
    for(const auto& node : graph.nodes){
        updateInstructionOrderNode(node,orderManager,visited);
    }
}

//Generate all of the graph for a file, (generate one for each function)
void generateGraph(const std::vector<std::vector<Instruction>>& graphVector,StmtOrder& orderManager){
    std::vector<Graph> graphs;
    for (auto& functionInstructions : graphVector)
    {
        
        auto graph = InstructionToGraph(functionInstructions);
        //PrintGraph(graph);
        optimizeGraph(graph);
        graphs.push_back(graph);
        updateInstructionOrderFromGraph(graph,orderManager);
    }
    GenerateDotGraph(graphs, "graph.dot");
}