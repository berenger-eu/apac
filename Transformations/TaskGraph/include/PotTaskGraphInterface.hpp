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
struct DependencyHash {
    std::size_t operator()(const std::pair<Access,const clang::NamedDecl*>& mt) const {
        std::hash<const void*> address_hash;
        std::hash<int> int_hash;
        return address_hash(mt.second) ^ int_hash(mt.first==Access::READ?1:0);
    }
};

struct DependencyEqual {
    bool operator()(const std::pair<Access,const clang::NamedDecl*>& lhs, const std::pair<Access,const clang::NamedDecl*>& rhs) const {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};
struct Instruction {
    clang::Stmt* instruction;
    std::string instructionString; //Instruction string
    bool complexInstruction;
    unsigned int scopedInstructionsNumber;  //Also takes in account number of instructions in ComplexInstructions 
    std::unordered_set<std::pair<Access, const clang::NamedDecl*>,DependencyHash,DependencyEqual> dependencies;
    std::vector<Instruction> scopedInstructions;
    void dumpDep() const{
        llvm::errs()<<"Dependencies for instruction: "<<instructionString<<"\n";
        for(const auto& dep:dependencies){
            std::stringstream ss;
            ss << (dep.first==Access::READ?"READ":"WRITE") << " " << dep.second->getNameAsString();
            llvm::errs()<<ss.str()<<"\n";
        }
    }
};

/*
struct ComplexInstruction : Instruction {
    std::vector<std::unique_ptr<Instruction>> scopedInstructions;
    unsigned int scopedInstructionsNumber;  //Also takes in account number of instructions in ComplexInstructions 
};*/


