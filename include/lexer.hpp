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

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glog/logging.h>

#include <system.hpp>

namespace hydra {

/**
 * Tokens are used to determine the different parts of a string.
 */
struct Token {
  Token() {}
  Token(const std::string &value) { this->value = value; }
  Token(const std::string &value, Type type) {
    this->value = value;
    this->type = type;
  }

  std::string value = "";
  Type type = Unknown;
  std::vector<Token> children = {};
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

class Lexer {
 public:

  Lexer(System &system);

  /**
   * The system knows about which functions and types are available,
   * which is important to know while parsing.
   */
  System &system;

  /**
   * Maps to each type the corresponding parser function.
   */
  std::unordered_map<
      Type,
      std::function<bool(Lexer *, const std::vector<Token> &, ParseResult &)>>
      known_parsers;

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
   * Determines whether a string consists of white spaces only.
   */
  static bool is_string_empty(std::string &str);

  /**
   * Finds the matching bracket to the one specified at position and
   * returns its index.  If the matching bracket was not found,
   * std::string::npos is returned.
   */
  static int position_of_matching_bracket_for_position(const std::string &str,
                                                       int position);

  /**
   * Finds the matching '"' to the one at the specified position and
   * returns its index.  If the matching quote was not found,
   * std::string::npos is returned.
   */
  static int position_of_matching_quote_for_position(const std::string &str,
                                                     int position);

  /**
   * Determines the tokens in a string.
   */
  bool tokenize_string(const std::string &str, std::vector<Token> &tokens);

  /**
   * Determines the type of a string.
   */
  Type type_of_string(const std::string &str);

  /**
   * Parses a tokenized string and tries to understand what it does. E.g.:
   * does this code represent an Assignment or a Function call, etc.
   */
  Type type_of_tokenized_string(const std::vector<Token> &tokenized_string);

  /**
   * Parses lines of code.
   */
  bool parse_code(const std::vector<std::string> &code,
                  std::vector<ParseResult> &parsed_code);

  /**
   * Parses a line of code.
   */
  bool parse_string(const std::string &str, ParseResult &result);

  /**
   * Parses a tokenized string.
   */
  bool parse_tokens(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses the parameter list of a function call or initialization.
   */
  bool parse_argument_list(const std::vector<Token> &tokens,
                           const std::vector<std::string> &expected_arguments,
                           ParseResult &result);

  /**
   * Parses a string that represents an assignment.
   */
  bool parse_assignment(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents an expression.  If the parsed
   * string is not an expression, the result will have type Error.
   */
  bool parse_expression(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents a function.  If the parsed string
   * is not a function call, the result will have type Error.
   */
  bool parse_function(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents an initialization.  If the parsed string
   * is not an initialization, the result will have type Error.
   */
  bool parse_initialization(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents the definition of a for-loop.  If
   * the parsed string is not a for-loop, the result will have type
   * Error.
   */
  bool parse_loop(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents a number.  If the parsed string
   * is not a number, the result will have type Error.
   */
  bool parse_number(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents a parenthesis.  If the parsed
   * string is not a parenthesis, the result will have type Error.
   */
  bool parse_parenthesis(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents a range.  If the parsed string
   * is not a number, the result will have type Error.
   */
  bool parse_range(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Parses a string that represents a string.
   */
  bool parse_string_token(const std::vector<Token> &tokens, ParseResult &result);

  /**
   * Recursively prints the passed parse result.
   */
  static void print_parse_result(const ParseResult &result,
                                 const std::string &indentation = "");

  /**
   * Recursively prints the passed tokenized string.
   */
  static void print_tokenized_string(const std::vector<Token> &tokenized_string,
                                     const std::string &indentation = "");
};
}  // namespace hydra

#endif /* lexer_hpp */
