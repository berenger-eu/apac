#pragma once
#include "AliasTable.hpp"
#include "Instruction.hpp"
#include "ParamWriteAnalyzer.hpp"

// Source convention: pointer = whole buffer, subscript = element,
// address-of-subscript = partition represented by its first element.
void analyzeArrayPhases(Instruction &, AliasTable &, const ParamWriteAnalyzer *,
                        const std::string &token);
bool finishArrayPhases(std::vector<Instruction> &);
