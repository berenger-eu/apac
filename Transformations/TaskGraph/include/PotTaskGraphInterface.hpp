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

struct AliasesDependencyHash {
    std::size_t operator()(const std::pair<const clang::VarDecl*,const clang::VarDecl*>& p) const {
        return std::hash<const clang::VarDecl*>{}(p.first) ^ std::hash<const clang::VarDecl*>{}(p.second);
    }
};
struct AliasesDependencyEqual {
    bool operator()(const std::pair<const clang::VarDecl*,const clang::VarDecl*>& p1, const std::pair<const clang::VarDecl*,const clang::VarDecl*>& p2) const {
        return p1.first == p2.first && p1.second == p2.second;
    }
};

struct Instruction {
    clang::Stmt* instruction;
    std::string instructionString; //Instruction string
    bool complexInstruction;
    unsigned int scopedInstructionsNumber;  //Also takes in account number of instructions in ComplexInstructions 
    std::unordered_map<const clang::NamedDecl*,NodeDependency> dependencies;
    std::vector<Instruction> scopedInstructions;
    //Contains pairs of variables
    //first element is the alias used in the instruction, second is the variable that is aliased
    std::unordered_set<std::pair<const clang::VarDecl*,const clang::VarDecl*>,AliasesDependencyHash,AliasesDependencyEqual> curAliases;
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


