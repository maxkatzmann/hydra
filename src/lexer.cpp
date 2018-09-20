//
//  lexer.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#include <lexer.hpp>

#include <iostream>
#include <stdexcept>

namespace hydra {

  Lexer::Lexer(System &system) : system(system) {}

void Lexer::components_in_string(const std::string &str,
                                std::vector<std::string> &components,
                                const std::string &delimiters) {
  /**
   * Skip delimiters at beginning.
   */
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  /**
   * Find first "non-delimiter".
   */
  std::string::size_type pos = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    /**
     * Found a token, add it to the vector.
     */
    components.push_back(str.substr(lastPos, pos - lastPos));

    /**
     * Skip delimiters.  Note the "not_of"
     */
    lastPos = str.find_first_not_of(delimiters, pos);

    /**
     * Find next "non-delimiter"
     */
    pos = str.find_first_of(delimiters, lastPos);
  }
}

void Lexer::clean_string(std::string &str) {
  /**
   * Find leading white spaces.
   */
  const std::size_t position_of_first_non_white_space =
      str.find_first_not_of(" \t");

  /**
   * If the first non-white space character was not found, the string is empty.
   */
  if (position_of_first_non_white_space == std::string::npos) {
    str = "";
  }

  /**
   * Remove leading spaces.
   */
  str.erase(0, position_of_first_non_white_space);

  /**
   * Find trailing white spaces.
   */
  const std::size_t position_of_last_non_white_space =
      str.find_last_not_of(" \t");

  /**
   * Remove leading and trailing white spaces.
   */
  if (position_of_last_non_white_space != std::string::npos) {
    str.erase(position_of_last_non_white_space + 1, std::string::npos);
  }

  /**
   * Remove comments.
   */
  std::size_t position_of_comment = str.find("//");

  /**
   * If a comment was found, erase everything behind the comment indicator.
   */
  if (position_of_comment != std::string::npos) {
    /**
     * Erase from comment till end of line.
     */
    str.erase(position_of_comment, std::string::npos);
  }
}

int Lexer::position_of_matching_bracket_for_position(const std::string &str,
                                                     int position) {
  const char opening_bracket = str[position];

  const std::unordered_map<char, char> bracket_pairs = {{'(', ')'}, {'[', ']'}, {'{', '}'}};

  const char closing_bracket = bracket_pairs.at(opening_bracket);

  /**
   * Before parsing the function arguments, find the closing
   * bracket.
   */
  int position_of_closing_bracket = -1;
  int number_of_open_brackets = 0;

  /**
   * Iterate the characters in the string to find the matching closing bracket.
   */
  for (int string_index = position; string_index < (int)str.length();
       ++string_index) {

    /**
     * Dealing open brackets.
     */
    if (str[string_index] == opening_bracket) {
      ++number_of_open_brackets;

      /**
       * Dealing with closing brackets.
       */
    } else if (str[string_index] == closing_bracket) {
      --number_of_open_brackets;

      /**
       * If we just closed the last bracket.
       */
      if (number_of_open_brackets == 0) {
        position_of_closing_bracket = string_index;
        break;
      }
    }
  }

  return position_of_closing_bracket;
}

bool Lexer::tokenize_string(const std::string &str, std::vector<Token> &tokens) {

  std::string cleaned_string = str;
  Lexer::clean_string(cleaned_string);

  DLOG(INFO) << "Tokenizing string: '" << cleaned_string << "'." << std::endl;

  std::string current_token;
  int current_index = 0;

  do {

    /**
     * Skip all white spaces.
     */
    current_index = cleaned_string.find_first_not_of(" \t", current_index);

    /**
     * If we found a bracket, we tokenize the contents of the bracket
     * as children of the last token.
     */
    if (cleaned_string[current_index] == '(' ||
        cleaned_string[current_index] == '[') {

      /**
       * Find the matching bracket.
       */
      int matching_bracket = Lexer::position_of_matching_bracket_for_position(
          cleaned_string, current_index);

      /**
       * If we didn't find a matching bracket, we have an error.
       */
      if (matching_bracket == (int)std::string::npos) {

        /**
         * Print the error message.
         */
        this->system.print_error_message(
            std::string("Missing parentheses: Could not find matching "
                        "parentheses for '") +
            cleaned_string[current_index] +
            "' at character index: " + std::to_string(current_index) + ".");

        Token token(System::error_string, type_of_string(System::error_string));
        tokens.push_back(token);
        return false;
      }

      /**
       * We're currently evaluating the content within brackets.  If
       * this resembles a function call or initialization, we add the
       * resulting tokens as children of the function token. Otherwise
       * we treat it as an expression or range.
       */
      if (tokens.back().type != Function &&
          tokens.back().type != Initialization) {
        /**
         * We don't have a function call. If we find '(' we are
         * probably dealing with an expression.
         */
        if (cleaned_string[current_index] == '(') {

          /**
           * We add the expression token to the end of the token
           * vector as the tokens within the parentheses will then
           * become children of this token.
           */
          Token expression_token("(", Expression);
          tokens.push_back(expression_token);

          /**
           * We don't have a function call. If we find '[' we are
           * probably dealing with a range.
           */
        } else if (cleaned_string[current_index] == '[') {
          /**
           * We add the range token to the end of the token vector as
           * the tokens within the brackets will then become children
           * of this token.
           */
          Token range_token("[", Range);
          tokens.push_back(range_token);
        }
      }

      /**
       * Tokenize the contents of the parentheses / brackets and add
       * them as children of the last token.
       */
      Lexer::tokenize_string(
                             cleaned_string.substr(current_index + 1,
                                                   matching_bracket - (current_index + 1)),
                             tokens.back().children);

      /**
       * Tokenize the content of the bracket as children of the token
       * we just created.
       */

      /**
       * We already tokenized the contents of in the bracket, so we
       * now continue with the contents after the bracket.
       */
      current_index = matching_bracket + 1;
    }

    /**
     * If the rest of the string consists of white spaces only, stop
     * tokenizing.
     */
    if (current_index == (int)std::string::npos) {
      break;
    }

    /**
     * Get the position of the next separator
     */
    int position_of_next_separator =
        cleaned_string.find_first_of(" (+-*/),:=[]", current_index);

    /**
     * If we can't find the next separator, we are at the end of the
     * string.  So we take the remainder of the strings as token.
     * Else we use the string up to the found separator as token.
     */
    if (position_of_next_separator == (int)std::string::npos) {
      current_token = cleaned_string.substr(current_index, std::string::npos);

      /**
       * If the current_index and the position_of_next_separator are
       * the same, we found a token only consisting of a separator.
       */
    } else if (position_of_next_separator == current_index) {
      /**
       * Take the 1 character only.
       */
      current_token = cleaned_string.substr(current_index, 1);
      /**
       * Make sure we continue with next character afterwards.
       */
      position_of_next_separator += 1;
    } else {
      current_token = cleaned_string.substr(
          current_index, position_of_next_separator - current_index);
    }

    Token token(current_token, type_of_string(current_token));

    /**
     * Continue with the next token.
     */
    current_index = position_of_next_separator;


    /**
     * Add the token, but only if either the token has a value or has
     * children.
     */
    if (!token.value.empty() || !token.children.empty()) {
      tokens.push_back(token);
    }

  } while (current_index != (int)std::string::npos &&
           current_index != (int)cleaned_string.length() - 1);

  return true;
}

Type Lexer::type_of_string(const std::string &str) {

  DLOG(INFO) << "Identifying token: '" << str << "'." << std::endl;

  if (str == System::error_string) {
    DLOG(INFO) << "Token identified as '"
               << System::name_for_type.at(Error) << "'."
               << std::endl;
    return Error;
  }

  /**
   * Check whether the token is keyword that we know of.
   */
  std::unordered_map<std::string, Type>::const_iterator token_type_result =
      this->system.types_for_keywords.find(str);

  if (token_type_result != this->system.types_for_keywords.end()) {

    DLOG(INFO) << "Token identified as '"
               << System::name_for_type.at(token_type_result->second) << "'."
               << std::endl;
    return token_type_result->second;

  } else {
    /**
     * Check whether we can cast the token to a number (or if it is
     * M_PI, which is also treated as a number).
     */
    if (str == "M_PI") {
      DLOG(INFO) << "Token identified as '" << System::name_for_type.at(Number)
                 << "'." << std::endl;
      return Number;
    } else {

      /**
       * Try casting as number.
       */
      try {
        std::stod(str);
        DLOG(INFO) << "Token identified as '" << System::name_for_type.at(Number)
                   << "'." << std::endl;
        return Number;
      } catch (std::invalid_argument &ia) {}
    }
  }

  return Unknown;
}

Type Lexer::type_of_tokenized_string(
    const std::vector<Token> &tokenized_string) {
  /**
   * Check again if the string is empty. If it is, it was probably a
   * comment.
   */
  if (tokenized_string.empty()) {
    DLOG(INFO)
        << "The tokenized string was empty. Probably because the original "
           "string was a comment. Identified as '"
        << System::name_for_type.at(Empty) << "'." << std::endl;
    return Empty;
  }

  /**
   * Every string that contains an '='-sign is an assignment.
   */
  for (const Token &token : tokenized_string) {
    if (token.value == "=") {
      DLOG(INFO) << "String contains '='. Identified as '"
                 << System::name_for_type.at(Assignment) << "'." << std::endl;
      return Assignment;
    }
  }

  /**
   * We don't have an assignment.  We now check whether there is an
   * operator somewhere.  Luckily, nested function calls and operators
   * are stored in children of tokens in this statement, so if we find
   * an operator, this whole string represents an expression.
   */
  for (const Token &token : tokenized_string) {
    /**
     * If we find an operator, we're fairly sure that we have an
     * expression.
     */
    if (token.type == Operator) {
      return Expression;
    }
  }

  /**
   * If it was not identified as an assignment, yet, therefore we use
   * the type of the first token.
   */
  return tokenized_string[0].type;
}

bool Lexer::parse_string(const std::string &str, ParseResult &result) {

  DLOG(INFO) << "Parsing string: '" << str << "'." << std::endl;

  /**
   * Tokenize the string.
   */
  std::vector<Token> tokens;
  if (tokenize_string(str, tokens)) {
#ifdef DEBUG
    print_tokenized_string(tokens);
#endif

    /**
     * Parse the tokenized string.
     */
    return parse_tokens(tokens, result);
  } else {

    /**
     * Return false if tokenizing the string failed.
     */
    return false;
  }
}

bool Lexer::parse_tokens(const std::vector<Token> &tokens,
                         ParseResult &result) {
  /**
   * Determine the type of the tokenized string.
   */
  Type type = type_of_tokenized_string(tokens);

  /**
   * Parse the tokenized string depending on the type.
   */
  switch (type) {
  case Assignment:
    return parse_assignment(tokens, result);
  case Expression:
    return parse_expression(tokens, result);
  case Function:
    return parse_function(tokens, result);
  case Initialization:
    return parse_initialization(tokens, result);
  case Number:
    return parse_number(tokens, result);
  default:
    result.type = Error;
    this->system.print_error_message(
        std::string("Could not parse line. Type of statement unclear."));
    return false;
  }
}

bool Lexer::parse_assignment(const std::vector<Token> &tokens,
                             ParseResult &result) {
  /**
   * The type is assignment
   */
  result.type = Assignment;

  /**
   * An Assignment needs to have the '=' operator as one token.  We
   * look for that token and determine the left and right hand side of
   * the expression.
   */
  int index_of_equality_sign = -1;
  std::vector<Token> lhs; // Tokens of the left hand side.
  std::vector<Token> rhs; // Tokens of the right hand side.

  for (int index = 0; index < (int)tokens.size(); ++index) {

    /**
     * Check whether we have found the equality sign.
     */
    if (tokens[index].value == "=") {

      /**
       * We can only have one = per assignment.
       */
      if (index_of_equality_sign < 0) {
        index_of_equality_sign = index;
      } else {
        result.type = Error;
        this->system.print_error_message(
            std::string("Invalid assignment. There must only be one assignment "
                        "per statement."));
        return false;
      }
    } else {
      /**
       * As long as we have not found the equality sign, we are on the
       * left hand side, otherwise we are on the right hand side.
       */
      if (index_of_equality_sign < 0) {
        lhs.push_back(tokens[index]);
      } else {
        rhs.push_back(tokens[index]);
      }
    }
  }

  /**
   * Parse left hand side.
   */

  /**
   * The index can only be 1 (a = 10.0) or 2 (var a = 10.0).
   */
  if (index_of_equality_sign == 1) {
    /**
     * Version: 'a = ...'
     */

    /**
     * We expect the left hand side to be variable name, so it should
     * be an unknown type.
     */
    if (lhs[0].type == Unknown) {

      /**
       * Add the parsed result.
       */
      ParseResult assignment_variable(Variable, lhs[0].value);
      result.children.push_back(assignment_variable);
    } else {
      result.type = Error;
      this->system.print_error_message(
          std::string(
              "Invalid assignment. Expected variable name but found '") +
          lhs[0].value +
          " instead. Use 'a = 10.0' or 'var a = 10.0' to assign a variable.");
      return false;
    }
  } else if (index_of_equality_sign == 2) {
    /**
     * Version: 'var a = ...'
     */

    /**
     * We expect a 'var' as first token.
     */
    if (lhs[0].value == "var") {
      /**
       * There are exactly two tokens. The first is 'var'. Now the
       * second has to be Unknown since it should be a variable name.
       */
      if (lhs[1].type == Unknown) {

        /**
         * Everything went as expected.
         */
        ParseResult assignment_keyword(Assignment, lhs[0].value);
        ParseResult assignment_variable(Variable, lhs[1].value);

        result.children.push_back(assignment_keyword);
        result.children.push_back(assignment_variable);
      } else {
        result.type = Error;
        this->system.print_error_message(
            std::string(
                "Invalid assignment. Expected variable name but found '") +
            lhs[0].value +
            " instead. Use 'a = 10.0' or 'var a = 10.0' to assign a variable.");
        return false;
      }
    } else {
      result.type = Error;
      this->system.print_error_message(std::string(
          "Invalid assignment. Use 'var a = 10.0' to declare a variable."));
      return false;
    }

  } else {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid assignment. Use 'a = 10.0' or 'var a = 10.0' to "
                    "assign a variable."));
    return false;
  }

  /**
   * Parse right hand side. (We only get here if the left hand side
   * was parsed correctly.)
   */
  ParseResult rhs_result;
  bool success = parse_tokens(rhs, rhs_result);
  result.children.push_back(rhs_result);

  if (!success) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Error while parsing right hand side of assignment."));
  }
  return success;
}

bool Lexer::parse_initialization(const std::vector<Token> &tokens,
                                 ParseResult &result) {

  /**
   * Check whether the first token signals an initialization.
   */
  if (tokens[0].type == Initialization) {

    /**
     * The type is initialization.
     */
    result.type = Initialization;
    result.value = tokens[0].value;

    /**
     * Get the list of expected arguments.
     */
    std::unordered_map<std::string, Func>::const_iterator
        position_of_arguments =
            this->system.known_functions.find(tokens[0].value);

    if (position_of_arguments != this->system.known_functions.end()) {
      /**
       * The arguments of an initialization are stored as children
       * of the initialization token.
       *
       * They must not be empty.
       */
      if (!tokens[0].children.empty()) {
        ParseResult argument_parse_result;
        bool success = Lexer::parse_argument_list(tokens[0].children,
                                                  position_of_arguments->second.arguments,
                                                  argument_parse_result);

        result.children.push_back(argument_parse_result);

        /**
         * If something went wrong, print usage of this
         * initializer.
         */
        if (!success) {
          this->system.print_error_message(
              std::string(
                  "An error occurred while parsing the argument list of '") +
              tokens[0].value + "'.");
          std::cerr << "> Usage of '" << tokens[0].value
                    << "': " << tokens[0].value << "(";
          System::print_argument_list(position_of_arguments->second.arguments);
          std::cerr << ")" << std::endl;
        }

        return success;
      } else {
        /**
         * If the argument list was empty, print an error.
         */
        result.type = Error;

        this->system.print_error_message(
            std::string("Missing arguments during initialization of '") +
            tokens[0].value + "'.");
        std::cerr << "> Usage of '" << tokens[0].value
                  << "': " << tokens[0].value << "(";
        System::print_argument_list(position_of_arguments->second.arguments);
        std::cerr << ")" << std::endl;
        return false;
      }
    }
  } else {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid initialization: '") + tokens[0].value +
        "' cannot be used to initialize a variable.");
    return false;
  }

  /**
   * If we haven't returned, yet, something went wrong.
   */
  return false;
}

bool Lexer::parse_number(const std::vector<Token> &tokens,
                         ParseResult &result) {

  /**
   * Check if the this is a single number.
   */
  if (tokens.size() == 1) {
    if (tokens[0].type == Number) {
      result.type = Number;
      result.value = tokens[0].value;
      return true;
    } else {
      result.type = Error;
      this->system.print_error_message(
          std::string("Invalid argument: '") + tokens[0].value +
          "' could not be read as '" + System::name_for_type.at(Number) + "'.");
      return false;
    }
  } else {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid number of arguments near '") + tokens[0].value +
        "'. Token could not be read as '" + System::name_for_type.at(Number) +
        "'.");
    return false;
  }
}

bool Lexer::parse_expression(const std::vector<Token> &tokens,
                             ParseResult &result) {
  /**
   * If the expression is empty, return false.
   */
  if (tokens.empty()) {
    result.type = Error;
    this->system.print_error_message(std::string(": Unexpectedly found empty expression."));
    return false;
  }

  /**
   * If there is only one token and its a number, than the result
   * simply the number, if the single token is the expression, then
   * its children contain the actual tokens.
   */
  if (tokens.size() == 1) {
    if (tokens[0].type == Number) {
      result.type = Number;
      result.value = tokens[0].value;
      return true;
    } else if (tokens[0].type == Expression && !tokens[0].children.empty()) {
      /**
       * In that case the children of the expression token represent
       * the actual expression.
       */
      return parse_expression(tokens[0].children, result);
    } else {
      result.type = Error;
      this->system.print_error_message(
          std::string("Invalid or empty expression."));
      return false;
    }

    /**
     * The number of tokens in an expression must be odd (term
     * operator term ...)
     */
  } else if (tokens.size() % 2 == 1) {
    /**
     * The expression is larger than one token.  Luckily, the tokens
     * of nested function calls are stored as children of the function
     * tokens, so we expected a list that has operators at the odd
     * positions.
     */
    result.type = Expression;

    for (int index = 0; index < (int)tokens.size(); ++index) {

      /**
       * Even indices must not be operators and need to be evaluated.
       */
      if (index % 2 == 0) {
        /**
         * Check if the token is an operator.
         */
        if (tokens[index].type != Operator) {
          /**
           * Evaluate the term.
           */
          ParseResult term_result;
          bool success = parse_tokens({tokens[index]}, term_result);

          result.children.push_back(term_result);

          if (!success) {
            result.type = Error;
            return false;
          }

        } else {
          result.type = Error;
          this->system.print_error_message(
              std::string("Invalid syntax: Unexpectedly found operator."));
          return false;
        }
      } else {
        /**
         * Odd indices are supposed to be operators.
         */
        if (tokens[index].type == Operator) {
          /**
           * We found an operator.
           */
          ParseResult operator_result(Operator, tokens[index].value);
          result.children.push_back(operator_result);

        } else {
          result.type = Error;
          this->system.print_error_message(
              std::string("Invalid syntax: Expected operator but found '") +
              tokens[index].value + "' instead.");
          return false;
        }
      }
    }

    /**
     * If we reach this point without returning, everything went well.
     */
    return true;
  } else {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid number of arguments in expression."));
    return false;
  }
}

bool Lexer::parse_function(const std::vector<Token> &tokens,
                           ParseResult &result) {

  /**
   * A function call consists of the function name only, with the arguments as children.
   */
  if (tokens.size() == 1) {

    /**
     * The first token should be a function.
     */
    if (tokens[0].type == Function) {

      result.type = Function;
      result.value = tokens[0].value;

      /**
       * The children of the function token are the arguments.
       */

      /**
       * Get the arguments for the function.
       */
      std::unordered_map<std::string, Func>::const_iterator
        position_of_function_arguments =
        this->system.known_functions.find(tokens[0].value);

      /**
       * Check whether we know this function.
       */
      if (position_of_function_arguments !=
          this->system.known_functions.end()) {

          /**
           * Parse the functions argument list
           */
          ParseResult argument_parse_result;
          bool success = parse_argument_list(tokens[0].children,
                              position_of_function_arguments->second.arguments,
                              argument_parse_result);
          result.children.push_back(argument_parse_result);

          /**
           * If we did not succeed parsing the argument list, print
           * the usage of the function.
           */
          if (!success) {
            result.type = Error;
            this->system.print_error_message(
                std::string("Invalid arguments in function call '") +
                tokens[0].value + "'.");
            std::cerr << "> Usage of '" << tokens[0].value
                      << "': " << tokens[0].value << "(";
            System::print_argument_list(
                position_of_function_arguments->second.arguments);
            std::cerr << ")" << std::endl;
          }

          return success;
      } else {
        /**
         * We don't know the function.
         */
        result.type = Error;
        this->system.print_error_message(std::string("Unknown function: '") +
                                         tokens[0].value + "'.");
        return false;
      }
    } else {
      result.type = Error;
      this->system.print_error_message(
          std::string("Invalid syntax. Expected function name but found '") +
          tokens[0].value + "', which is of type '" +
          System::name_for_type.at(tokens[0].type));
      return false;
    }
  } else {
    result.type = Error;
    this->system.print_error_message(std::string(
        "Invalid number of statements. Use only one function call per line."));
    return false;
  }

  /**
   * If we didn't return, yet, something went wrong.
   */
  return false;
}

bool Lexer::parse_argument_list(
    const std::vector<Token> &tokens,
    const std::vector<std::string> &expected_arguments, ParseResult &result) {

  result.type = ArgumentList;

  /**
   * Iterate the argument list and check whether it matches the
   * expected arguments.
   */
  int number_of_found_arguments = 0;

  /**
   * If the argument list is empty, something went wrong.
   */
  if (!tokens.empty()) {

    /**
     * The argument list has the format:
     * 'argument_name: argument_values ..., argument_name: argument_values ...'
     *
     * We proceed as follows. Get the argument name and check whether
     * it matches the expected argument. Then check whether the
     * argument is followed by ':'. Then gather the argument values.
     */
    int token_index = 0;
    while (token_index < (int)tokens.size()) {

      /**
       * First check whether we have not already found more arguments
       * than are expected.
       */
      if (number_of_found_arguments < (int)expected_arguments.size()) {
        /**
         * Check whether the argument matches the expected argument.
         */
        std::string argument_name = tokens[token_index].value;
        if (argument_name ==
            expected_arguments[number_of_found_arguments]) {

          /**
           * Arguments match. Next up: check whether the next token is
           * a ':'. Thus, check if we're not already at the end of the
           * argument list.
           */
          ++token_index;
          if (token_index < (int)tokens.size()) {
            /**
             * Check if this next token is a ':'.
             */
            if (tokens[token_index].value == ":") {

              /**
               * Now collect the argument values. First check again if
               * we're not at the end of the argument list.
               */
              ++token_index;

              if (token_index < (int)tokens.size()) {

                /**
                 * Luckily, function calls in argument values have
                 * their arguments as children and thus do not appear
                 * in this argument list. Therefore, we only need to
                 * look for the next ',' or the end of the token list.
                 */
                std::vector<Token> argument_value_tokens;

                /**
                 * While we have not found a ',' or the end of the
                 * token list, collect tokens for the current argument
                 * value.
                 */
                while (tokens[token_index].value != "," &&
                       token_index < (int)tokens.size()) {
                  /**
                   * When we find a ':' before finding a ',' or the end
                   * of the token list, the syntax is invalid.
                   */
                  if (tokens[token_index].value == ":") {
                    result.type = Error;
                    this->system.print_error_message(
                        std::string("Invalid syntax in function call. Expected "
                                    "',' but found ':' instead."));
                    return false;
                  }

                  /**
                   * Add the token to the current list, and continue.
                   */
                  argument_value_tokens.push_back(tokens[token_index]);
                  ++token_index;
                }

                /**
                 * We have now collected all values for this argument.  Parse them!
                 */
                ParseResult argument_result(Argument, argument_name);

                /**
                 * Parse the argument value.
                 */
                ParseResult argument_evaluation;
                bool success = parse_tokens(argument_value_tokens, argument_evaluation);

                /**
                 * If something went wrong, return false;
                 */
                if (!success) {
                  result.type = Error;
                  return false;
                }

                /**
                 * Add the parsed results.
                 */
                argument_result.children.push_back(argument_evaluation);
                result.children.push_back(argument_result);

                /**
                 * We have just identified an argument.
                 */
                ++number_of_found_arguments;

                /**
                 * At this point we're either at a ',' or at the end
                 * of the argument list. We increase the
                 * token_index. If we're at a ',' we should then be at
                 * the next argument, otherwise we just stop anyway.
                 */
                ++token_index;
              } else {
                result.type = Error;
                this->system.print_error_message(
                    std::string("Missing argument value in function call."));
                return false;
              }
            } else {
              result.type = Error;
              this->system.print_error_message(
                  std::string("Invalid syntax in function call. Expected ':' "
                              "but found '") +
                  tokens[token_index].value + "' instead.");
              return false;
            }

          } else {
            result.type = Error;
            this->system.print_error_message(
                std::string("Missing argument value in function call."));
            return false;
          }

        } else {
          result.type = Error;
          this->system.print_error_message(
              std::string("Invalid argument in function call. Expected '") +
              expected_arguments[number_of_found_arguments] + "' but found '" +
              tokens[token_index].value + "' instead.");
          return false;
        }
      } else {
        result.type = Error;
        this->system.print_error_message(
            std::string("Extraneous argument in function call."));
        return false;
      }
    }

    /**
     * When we arrived here, we parsed the whole argument list
     * successfully.
     */
    return true;
  } else {
    result.type = Error;
    this->system.print_error_message(
        std::string("Missing argument in function call."));
    return false;
  }
}

void Lexer::print_parse_result(const ParseResult &result,
                               const std::string &indentation) {
  std::cout << indentation << System::name_for_type.at(result.type) << ": "
            << result.value << std::endl;
  for (const ParseResult &child_result : result.children) {
    Lexer::print_parse_result(child_result, indentation + "\t");
  }
}

void Lexer::print_tokenized_string(const std::vector<Token> &tokenized_string,
                                   const std::string &indentation) {
  for (const Token &token : tokenized_string) {
    std::cout << indentation << token.value << " ("
              << System::name_for_type.at(token.type) << ")" << std::endl;
    Lexer::print_tokenized_string(token.children, indentation + "\t");
  }
}
}  // namespace hydra
