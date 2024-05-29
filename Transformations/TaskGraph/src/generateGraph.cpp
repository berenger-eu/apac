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

auto InstructionToGraph(const std::vector<Instruction>& inInstructions){
    Graph graph;
    std::stack<const Instruction*> instructionStack;
    for (auto it = inInstructions.rbegin(); it != inInstructions.rend(); ++it){
        const Instruction* instr = &(*it);
        instructionStack.push(instr);
    }
    while (!instructionStack.empty()){
        const Instruction*& curInstruction =instructionStack.top();
        instructionStack.pop();
        auto node = std::make_shared<Node>();
        node->instruction = curInstruction->instructionString;//instruction.instruction;
        node->id = graph.nodes.size();
        graph.nodes.push_back(node);
        if(curInstruction->complexInstruction){
            for (auto it = curInstruction->scopedInstructions.rbegin(); it != curInstruction->scopedInstructions.rend(); ++it){
                const Instruction* instr = &(*it);
                instructionStack.push(instr);
            }
        }
    }
    std::unordered_map<const clang::Decl*, std::set<std::shared_ptr<Node>>> dataUsedInRead;
    std::unordered_map<const clang::Decl*, std::shared_ptr<Node>> dataUsedInWrite;

    for (int i = 0; i < inInstructions.size(); ++i){
        auto node = graph.nodes[i];
        for (const auto& dep : inInstructions[i].dependencies){
            if (dep.first == Access::READ){
                if(dataUsedInWrite.find(dep.second) != dataUsedInWrite.end()&&dataUsedInWrite[dep.second]->id!=node->id){
                    auto depNode = dataUsedInWrite[dep.second];
                    depNode->next.insert(node);
                    node->prev.insert(depNode);
                }
                dataUsedInRead[dep.second].insert(node);
            } 
            else {
                if(dataUsedInRead.find(dep.second) != dataUsedInRead.end()){
                    for (const auto& depNode : dataUsedInRead[dep.second]){
                        if(depNode->id == node->id){
                            continue;
                        }
                        depNode->next.insert(node);
                        node->prev.insert(depNode);
                    }

                    dataUsedInRead[dep.second].clear();
                }
                else if(dataUsedInWrite.find(dep.second) != dataUsedInWrite.end()&&dataUsedInWrite[dep.second]->id!=node->id){
                    auto depNode = dataUsedInWrite[dep.second];
                    depNode->next.insert(node);
                    node->prev.insert(depNode);
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
    }
    std::cout << "Roots: " << std::endl;
    for(const auto& root : inGraph.roots)
        std::cout << root->id <<" ";
    std::cout << std::endl;
}

//Generate all of the graph for a file, (generate one for each function)
void generateGraph(const std::vector<std::vector<Instruction>>& graphVector){
    for (auto& functionInstructions : graphVector)
    {

        auto graph = InstructionToGraph(functionInstructions);
        PrintGraph(graph);
    }
}