//
//  system.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 19.09.18.
//

#include <system.hpp>

#include <iostream>

namespace hydra {

const std::string System::error_string = "__ERROR__";

const std::unordered_map<Type, std::string, std::hash<int>>
    System::name_for_type = {
        {Argument, "Argument"},
        {ArgumentList, "ArgumentList"},
        {Assignment, "Assignment"},
        {Empty, "Empty"},
        {Error, "Error"},
        {Expression, "Expression"},
        {Function, "Function"},
        {Initialization, "Initialization"},
        {Loop, "Loop"},
        {Number, "Number"},
        {Operator, "Operator"},
        {Range, "Range"},
        {Unknown, "Unknown"},
        {Variable, "Variable"}};

const std::unordered_map<std::string, Type> System::types_for_keywords = {
    {"arc", Function},
    {"circle", Function},
    {"cos", Function},
    {"cosh", Function},
    {"curve_angle", Function},
    {"curve_distance", Function},
    {"Euc", Initialization},
    {"exp", Function},
    {"for", Loop},
    {"in", Range},
    {"let", Assignment},
    {"line", Function},
    {"Pol", Initialization},
    {"point", Function},
    {"random", Function},
    {"save", Function},
    {"sin", Function},
    {"sinh", Function},
    {"show", Function},
    {"theta", Function},
    {"+", Operator},
    {"-", Operator},
    {"*", Operator},
    {"/", Operator},
    {"=", Assignment}};

const std::unordered_map<std::string, std::vector<std::string>>
    System::arguments_for_functions = {
        {"arc", {"center", "radius", "from", "to"}},
        {"circle", {"center", "radius"}},
        {"cos", {"x"}},
        {"cosh", {"x"}},
        {"curve_angle", {"from", "to", "angle"}},
        {"curve_distance", {"from", "to", "distance"}},
        {"Euc", {"x", "y"}},
        {"exp", {"x"}},
        {"line", {"from", "to"}},
        {"point", {"center", "radius"}},
        {"Pol", {"r", "phi"}},
        {"random", {"from", "to"}},
        {"save", {"file"}},
        {"sin", {"x"}},
        {"sinh", {"x"}},
        {"show", {}},
        {"theta", {"r1", "r2", "R"}}};

void System::print_argument_list(const std::vector<std::string> &arguments) {
  for (int i = 0; i < (int)arguments.size(); ++i) {
    std::cerr << arguments[i];

    if (i < (int)arguments.size() - 1) {
      std::cerr << ":";
    }
  }
}
} // namespace hydra
