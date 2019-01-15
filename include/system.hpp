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
  FunctionDefinition = 7,
  Loop = 8,
  Initialization = 9,
  Number = 10,
  Operator = 11,
  Braces = 12,
  Parameter = 13,
  ParameterList = 14,
  Property = 15,
  Range = 16,
  String = 17,
  StringEscape = 18,
  Unknown = 19,
  Variable = 20
};

/**
 * A parse result has a type, e.g. Assignment, a value (which is the
 * original string that yielded the result) and a vector
 * representing the child results of the parsing.
 */
struct ParseResult {
  ParseResult() : type(Unknown), value(""), children({}) {}

  ParseResult(Type type, const std::string &value) {
    this->type = type;
    this->value = value;
  }

  Type type = Unknown;
  std::string value = "";
  std::vector<ParseResult> children = {};

  /**
   * A parse result is associated with the number of the line from
   * which it was parsed.
   */
  int line_number = -1;
};

/**
 * We use unordered_maps from string to any to store properties of
 * non-primitive types.
 */
typedef std::unordered_map<std::string, std::any> PropertyMap;

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
  static const std::string type_string;

  /**
   * Constructor
   */
  System();

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
  * In order to execute user defined functions, we need to store the
  * statement for the corresponding function.
  */
 std::unordered_map<std::string, std::vector<ParseResult>>
     statements_for_functions;

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
