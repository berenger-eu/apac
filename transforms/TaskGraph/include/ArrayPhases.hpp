#pragma once
#include "AliasTable.hpp"
#include "Instruction.hpp"
#include "ParamWriteAnalyzer.hpp"

// Source convention: pointer = whole buffer, subscript = element,
// address-of-subscript = partition represented by its first element.
void analyzeArrayPhases(Instruction &, AliasTable &, const ParamWriteAnalyzer *,
                        const std::string &token);
bool finishArrayPhases(std::vector<Instruction> &);
// Lower simple loop nests of calls with explicit partition representatives.
// On failure no source is changed; the caller keeps its synchronous fallback.
bool lowerPartitionCallLoop(clang::ForStmt *, AliasTable &,
                            const ParamWriteAnalyzer *, std::string &);
