#pragma once
#include <sstream>
#include <vector>
#include <unordered_set>
#include "clang/AST/Stmt.h"
enum class Access {
    READ,
    WRITE
};
struct DependencyHash {
    std::size_t operator()(const std::pair<Access,std::string>& mt) const {
        std::hash<std::string> string_hash;
        std::hash<int> int_hash;
        return string_hash(mt.second) ^ int_hash(mt.first==Access::READ?1:0);
    }
};

struct DependencyEqual {
    bool operator()(const std::pair<Access,std::string>& lhs, const std::pair<Access,std::string>& rhs) const {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};
struct Instruction {
    std::string instructionString; //Instruction string
    clang::Stmt* instruction;
    std::unordered_set<std::pair<Access, std::string>,DependencyHash,DependencyEqual> dependencies;
};
struct ComplexInstruction : Instruction {
    std::vector<Instruction> scopedInstructions;
    unsigned scopedInstructionsNumber;  //Also takes in account number of instructions in ComplexInstructions 
};


