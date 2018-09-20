//
//  system.hpp
//  hydra
//
//  Represents the hydra system.
//
//  Created by Maximilian Katzmann on 19.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef system_hpp
#define system_hpp

#include <any>
#include <string>
#include <unordered_map>
#include <vector>

#include <state.hpp>

namespace hydra {

enum Type {
  Argument = 0,
  ArgumentList = 1,
  Assignment = 2,
  Empty = 3,
  Error = 4,
  Expression = 5,
  Function = 6,
  Loop = 7,
  Initialization = 8,
  Number = 9,
  Operator = 10,
  Range = 11,
  Unknown = 12,
  Variable = 13
};

/**
 * How functions are represented in hydra.
 */
struct Func {
  Func(const std::string &name) { this->name = name; }

  Func(const std::string &name, const std::vector<std::string> &arguments) {
    this->name = name;
    this->arguments = arguments;
  }

  std::string name = "";
  std::vector<std::string> arguments = {};  // The arguments of a function.

  /**
   * When interpreting the parameters of a functions, we need to be
   * able to assign the parameters to variables within the
   * function. This map tells us how.
   */
  std::unordered_map<std::string, std::string> parameter_map = {};
};

class System {

 public:

  static const std::string error_string;

  /**
   * Constructor
   */
  System();

  /**
   * This doesn't feel right but I have no better place, yet, so the
   * system stores the current line number.
   */
  int line_number = -1;

  /**
   * Also not sure whether the system should
   */
  std::string current_line = "";

  /**
   * The system has a state that encapsulate what the program has seen
   * so far / is currently seeing.
   */
  State state;

 /**
  * Assigns each type the corresponding name.
  */
 static const std::unordered_map<Type, std::string, std::hash<int>>
     name_for_type;

 /**
  * Contains all known keywords and their associated types.  E.g.:
  * "var" is of type Assignment.
  *
  * This is not static since we later want to allow the addition of
  * custom functions.
  */
 std::unordered_map<std::string, Type> types_for_keywords;

 /**
  * Contains all known functions and the associated arguments. E.g.:
  * "line -> from:to:"
  */
 std::unordered_map<std::string, Func> known_functions;

 /**
  * Prints an error message to the console.
  */
 void print_error_message(const std::string &message);

 /**
  * Prints a vector of strings a argument list.
  */
 static void print_argument_list(const std::vector<std::string> &arguments);
};

} // namespace hydra

#endif /* system_hpp */
