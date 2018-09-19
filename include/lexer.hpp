//
//  lexer.hpp
//  hydra
//
//  Used to parse hydra code.
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef lexer_hpp
#define lexer_hpp

#include <string>
#include <unordered_map>
#include <vector>

#include <glog/logging.h>

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
  Unknown = 10,
  Variable = 11
};

/**
 * A parse result has a type, e.g. Assignment, a value (which is the
 * original string that yielded the result) and a vector
 * representing the child results of the parsing.
 */
struct ParseResult {

  ParseResult() : type(Unknown), value(""), children({}) {}

  ParseResult(Type type, std::string &value) {
    this->type = type;
    this->value = value;
  }

  Type type = Unknown;
  std::string value = "";
  std::vector<ParseResult> children = {};
};

class Lexer {
 public:
  int line_number = 0;

  Lexer() {}

  /**
   * Assigns each type the corresponding name.
   */
  static const std::unordered_map<Type, std::string, std::hash<int>>
      name_for_type;

  /**
   * Contains all known keywords and their associated types.  E.g.:
   * "let" is of type Assignment.
   */
  static const std::unordered_map<std::string, Type> types_for_keywords;

  /**
   * For a given string determines the corresponding Type, or
   * VariableOrUnknown if no Type matched.
   */
  static Type type_for_string(const std::string &str);

  /**
   * Contains all known functions and the associated arguments. E.g.:
   * "line -> from:to:"
   */
  static const std::unordered_map<std::string, std::vector<std::string>>
      arguments_for_functions;

  /**
   * Determines the components in a string separated by the passed
   * delimiters.
   */
  static void components_in_string(const std::string &str,
                                   std::vector<std::string> &components,
                                   const std::string &delimiters);

  /**
   * Cleans a string by removing leading and trailing white spaces as
   * well as comments.
   */
  static void clean_string(std::string &str);

  /**
   * Determines the first token in a string.
   */
  static void first_token_in_string(const std::string &str, std::string &token);

  /**
   * Finds the matching bracket to the one specified at position and
   * returns its index.  If the matching bracket was not found,
   * std::string::npos is returned.
   */
  static int position_of_matching_bracket_for_position(const std::string &str,
                                                       int position);

  /**
   * Parses a line of code and tries to understand what it does. E.g.:
   * does this code represent an Assignment or a Function call, etc.
   */
  Type identify_string(const std::string &str);

  /**
   * Parses a string.
   */
  void parse_string(const std::string &str, ParseResult &result);

  /**
   * Parses a string that represents an assignment.
   */
  void parse_assignment(const std::string &str, ParseResult &result);

  /**
   * Parses a string that represents an initialization.  If the parsed string
   * is not an initialization, the result will have type Error.
   */
  void parse_initialization(const std::string &str, ParseResult &result);

  /**
   * Parses a string that represents a number.  If the parsed string
   * is not a number, the result will have type Error.
   */
  void parse_number(const std::string &str, ParseResult &result);

  /**
   * Parses a string that represents a function.  If the parsed string
   * is not a function call, the result will have type Error.
   */
  void parse_function(const std::string &str, ParseResult &result);

  /**
   * Parses the parameter list of a function call or initialization.
   */
  void parse_argument_list(const std::string &str,
                           const std::vector<std::string> &expected_arguments,
                           ParseResult &result);

  /**
   * Prints a vector of strings a argument list.
   */
  static void print_argument_list(const std::vector<std::string> &arguments);

  /**
   * Recursively prints the passed parse result.
   */
  static void print_parse_result(const ParseResult &result,
                                 const std::string &indentation = "");
};
}  // namespace hydra

#endif /* lexer_hpp */
