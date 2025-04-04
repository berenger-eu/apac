#include <iostream>
#include <sstream>
#include <string>
#include <vector>
inline void callParse(std::string &main, std::string &functions,
                      std::string &ignore, std::string &mainName,
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

inline bool isToParseFunction(const std::string &function,
                              const std::vector<std::string> &functionsToParse,
                              const std::vector<std::string> &functionsToIgnore,
                              const std::string &main) {
  // If it's one of our internal functions, we don't parse it
  if (function.find("_apacSeq") != std::string::npos)
    return false;
  // Check if the function is in the ignore list
  for (const auto &f : functionsToIgnore) {
    if (function == f) {
      return false;
    }
  }
  // By default, we parse all functions
  if (functionsToParse.empty())
    return true;
  for (const auto &f : functionsToParse) {
    if (function == f) {
      return true;
    }
  }
  // We always parse the main function unless it is in the ignore list
  if (function == main) {
    return true;
  }
  return false;
}