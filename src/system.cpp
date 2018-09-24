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
        {Parenthesis, "Parenthesis"},
        {Range, "Range"},
        {String, "String"},
        {StringEscape, "StringEscape"},
        {Unknown, "Unknown"},
        {Variable, "Variable"}};

System::System() {

  this->types_for_keywords = {{"arc", Function},
                              {"circle", Function},
                              {"clear", Function},
                              {"cos", Function},
                              {"cosh", Function},
                              {"curve_angle", Function},
                              {"curve_distance", Function},
                              {"Euc", Initialization},
                              {"exp", Function},
                              {"for", Loop},
                              {"in", Range},
                              {"line", Function},
                              {"mark", Function},
                              {"Pol", Initialization},
                              {"print", Function},
                              {"random", Function},
                              {"save", Function},
                              {"sin", Function},
                              {"sinh", Function},
                              {"show", Function},
                              {"theta", Function},
                              {"var", Assignment},
                              {"+", Operator},
                              {"-", Operator},
                              {"*", Operator},
                              {"/", Operator},
                              {"=", Assignment},
                              {"{", Parenthesis},
                              {"}", Parenthesis}};

  /**
   * The initially known functions.
   */
  this->known_functions = {{"arc", Func("arc", {"center", "radius", "from", "to"})},
                           {"circle", Func("circle", {"center", "radius"})},
                           {"clear", Func("clear", {})},
                           {"cos", Func("cos", {"x"})},
                           {"cosh", Func("cosh", {"x"})},
                           {"curve_angle", Func("curve_angle", {"from", "to", "angle"})},
                           {"curve_distance", Func("curve_distance", {"from", "to", "distance"})},
                           {"Euc", Func("Euc", {"x", "y"})},
                           {"exp", Func("exp", {"x"})},
                           {"line", Func("line", {"from", "to"})},
                           {"mark", Func("mark", {"center", "radius"})},
                           {"Pol", Func("Pol", {"r", "phi"})},
                           {"print", Func("print", {"message"})},
                           {"random", Func("random", {"from", "to"})},
                           {"save", Func("save", {"file"})},
                           {"sin", Func("sin", {"x"})},
                           {"sinh", Func("sinh", {"x"})},
                           {"show", Func("show", {})},
                           {"theta", Func("theta", {"r1", "r2", "R"})}};
}

void System::print_error_message(const std::string &message) {
  if (this->state.line_number >= 0) {
    if (!this->state.current_line.empty()) {
      std::cerr << "Error in line " << this->state.line_number << ": '"
                << this->state.current_line << "'." << std::endl
                << "> " << message << std::endl;
    } else {
      std::cerr << "Error in line " << this->state.line_number << ": "
                << message << std::endl;
    }
  } else {
    std::cerr << "> " << message << std::endl;
  }
}

void System::print_argument_list(const std::vector<std::string> &arguments) {
  for (int i = 0; i < (int)arguments.size(); ++i) {
    std::cerr << arguments[i];

    if (i < (int)arguments.size() - 1) {
      std::cerr << ":";
    }
  }
}
} // namespace hydra
