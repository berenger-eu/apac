#pragma once
#include "APACRecursiveASTVisitor.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
inline void callParse(const std::string &main, const std::string &functions,
                      const std::string &ignore, std::string &mainName,
                      std::vector<std::string> &functionsToIgnore,
                      std::vector<std::string> &functionsToParse) {
  mainName = main.empty() ? "main" : main;
  if (!functions.empty()) {
    std::stringstream ss(functions);
    std::string function;
    while (std::getline(ss, function, ',')) {
      functionsToParse.push_back(function);
    }
  }
  if (!ignore.empty()) {
    std::stringstream ss(ignore);
    std::string function;
    while (std::getline(ss, function, ',')) {
      functionsToIgnore.push_back(function);
    }
  }
}
