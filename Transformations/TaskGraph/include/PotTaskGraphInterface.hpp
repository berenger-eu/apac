#pragma once
#include <sstream>
#include <vector>
#include <unordered_set>
#include "clang/AST/Stmt.h"
#include "clang/AST/Decl.h"
enum class Access {
    READ,
    WRITE
};
struct NodeDependency {
    bool isRead;
    bool isWrite;
};

struct Instruction {
    clang::Stmt* instruction;
    std::string instructionString; //Instruction string
    bool complexInstruction;
    unsigned int scopedInstructionsNumber;  //Also takes in account number of instructions in ComplexInstructions 
    std::unordered_map<const clang::NamedDecl*,NodeDependency> dependencies;
    std::vector<Instruction> scopedInstructions;
    void dumpDep() const{
        llvm::errs()<<"Dependencies for instruction: "<<instructionString<<"\n";
        for(const auto& dep:dependencies){
            std::stringstream ss;
            ss << (dep.second.isRead?"READ ":"") << (dep.second.isWrite?"WRITE ":"")  << " " << dep.first->getNameAsString();
            llvm::errs()<<ss.str()<<"\n";
        }
    }
};

/*
struct ComplexInstruction : Instruction {
    std::vector<std::unique_ptr<Instruction>> scopedInstructions;
    unsigned int scopedInstructionsNumber;  //Also takes in account number of instructions in ComplexInstructions 
};*/


