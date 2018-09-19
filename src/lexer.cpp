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

const std::unordered_map<Type, std::string, std::hash<int>>
    Lexer::name_for_type = {{Argument, "Argument"},
                            {ArgumentList, "ArgumentList"},
                            {Assignment, "Assignment"},
                            {Empty, "Empty"},
                            {Error, "Error"},
                            {Expression, "Expression"},
                            {Function, "Function"},
                            {Initialization, "Initialization"},
                            {Number, "Number"},
                            {Unknown, "Unknown"},
                            {Variable, "Variable"}};

const std::unordered_map<std::string, Type> Lexer::types_for_keywords = {
    {"arc", Function},
    {"circle", Function},
    {"curve_angle", Function},
    {"curve_distance", Function},
    {"Euc", Initialization},
    {"let", Assignment},
    {"line", Function},
    {"Pol", Initialization},
    {"point", Function},
    {"save", Function},
    {"show", Function},
    {"theta", Function}};

Type Lexer::type_for_string(const std::string &str) {
  /**
   * First we check, whether the passed string is a known keyword.
   */
  std::unordered_map<std::string, Type>::const_iterator keyword_search_result =
      Lexer::types_for_keywords.find(str);

  /**
   * If string is a keyword, we return the corresponding type.
   */
  if (keyword_search_result != Lexer::types_for_keywords.end()) {
    /**
     * If the keyword was found, we return it.
     */
    return keyword_search_result->second;
  } else {
    /**
     * If the string is not a keyword, we try to determine the type.
     */

    /**
     * Check whether we have a double value
     */
    try {
      std::stod(str);  // Try to cast to double
      return Number;   // Return Number if cast was successful.
    } catch (const std::invalid_argument &ia) {

      /**
       * The string was not a keyword or a number. So we don't know
       * what it is.
       */
      return Unknown;
    }
  }
}

const std::unordered_map<std::string, std::vector<std::string>>
    Lexer::arguments_for_functions = {
        {"arc", {"center", "radius", "from", "to"}},
        {"circle", {"center", "radius"}},
        {"curve_angle", {"from", "to", "angle"}},
        {"curve_distance", {"from", "to", "distance"}},
        {"Euc", {"x", "y"}},
        {"line", {"from", "to"}},
        {"point", {"center", "radius"}},
        {"Pol", {"r", "phi"}},
        {"save", {"file"}},
        {"show", {}},
        {"theta", {"r1", "r2", "R"}}};

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

void Lexer::first_token_in_string(const std::string &str, std::string &token) {
  /**
   * Get the very first token.
   */
  std::size_t separator_position = str.find_first_of(" (/-+*");

  if (separator_position != std::string::npos) {

    /**
     * Get the first token and try to get its type.
     */
    token = str.substr(0, separator_position);
  } else {
    token = str;
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

Type Lexer::identify_string(const std::string &str) {

  /**
   * Clean the string by removing all leading white spaces and comments.
   */
  std::string cleaned_string = str;
  Lexer::clean_string(cleaned_string);

  DLOG(INFO) << "Identifying string: '" << str << "'." << std::endl;

  /**
   * Check again if the cleaned string is empty. If it is, it was
   * probably a comment.
   */
  if (cleaned_string.empty()) {
    DLOG(INFO) << "The cleaned string was empty. Probably because the original "
                  "string was a comment. Identified as '"
               << Lexer::name_for_type.at(Empty) << "'." << std::endl;
    return Empty;
  }

  /**
   * Every string that contains an '='-sign is an assignment.
   */
  if (cleaned_string.find('=') != std::string::npos) {
    DLOG(INFO) << "String contains '='. Identified as '"
               << Lexer::name_for_type.at(Assignment) << "'." << std::endl;
    return Assignment;
  } else {

    std::string first_token;
    Lexer::first_token_in_string(cleaned_string, first_token);

    if (!first_token.empty()) {
      Type type = Lexer::type_for_string(first_token);
      DLOG(INFO) << "String identified as '" << Lexer::name_for_type.at(type)
                 << "'." << std::endl;
      return type;
    }

    DLOG(INFO) << "String could not be identified. Identified as '"
               << Lexer::name_for_type.at(Unknown) << "'." << std::endl;
    return Unknown;
  }
}

void Lexer::parse_string(const std::string &str, ParseResult &result) {

  DLOG(INFO) << "Parsing string: '" << str << "'." << std::endl;

  Type type = identify_string(str);

  switch (type) {
  case Assignment:
    parse_assignment(str, result);
    break;
  case Function:
    parse_function(str, result);
    break;
  case Initialization:
    parse_initialization(str, result);
    break;
  case Number:
    parse_number(str, result);
    break;
  default:
    result.type = Error;
    DLOG(INFO) << "Could not parse '" << str << "', which seems to be of type '"
               << Lexer::name_for_type.at(type) << "'." << std::endl;
    break;
  }
}

void Lexer::parse_assignment(const std::string &str,
                             ParseResult &result) {

  /**
   * Clean the string.
   */
  std::string cleaned_string = str;
  Lexer::clean_string(cleaned_string);

  DLOG(INFO) << "Parsing assignment: '" << cleaned_string << "'." << std::endl;

  /**
   * The type is assignment and the string is its value.
   */
  result.type = Assignment;
  result.value = cleaned_string;

  /**
   * Determine left and right hand side.
   */
  std::size_t position_of_equality_sign = cleaned_string.find('=');
  if (position_of_equality_sign != std::string::npos) {

    /**
     * Get left and right side.
     */
    std::string lhs = cleaned_string.substr(0, position_of_equality_sign);
    std::string rhs =
        cleaned_string.substr(position_of_equality_sign + 1, std::string::npos);

    /**
     * Parse left hand side.
     */
    DLOG(INFO) << "Parsing left hand side." << std::endl;
    Lexer::clean_string(lhs);
    Type left_hand_type = Lexer::identify_string(lhs);

    /**
     * The type of the left hand side should either by 'Unknown' (if a
     * variable is being assigned to) or 'Assignment' if a new
     * variable is being defined.
     */
    if (left_hand_type == Assignment) {
      /**
       * If the lhs is an assignment, it consists of exactly two
       * tokens: 'let' followed by the variable name.
       */
      std::vector<std::string> lhs_components;
      Lexer::components_in_string(lhs, lhs_components, " \t");

      /**
       * The assignment is only valid, if it is of the form: 'let variable'
       */
      if (lhs_components.size() == 2 && lhs_components[0] == "let") {

        ParseResult assignment_keyword(Assignment, lhs_components[0]);
        ParseResult assignment_variable(Variable, lhs_components[1]);

        result.children.push_back(assignment_keyword);
        result.children.push_back(assignment_variable);

      } else {
            std::cerr << "Line " << this->line_number << ": Invalid assignment expression: '" << lhs
                   << " = ... '. Use 'let variable = ...' instead."
                   << std::endl;
      }
    } else if (left_hand_type == Unknown) {
      /**
       * In an assignment the left hand side is of type unknown, if it
       * is a variable name. In that case we expect only one token,
       * which is the variable name.
       */
      std::vector<std::string> lhs_components;
      Lexer::components_in_string(lhs, lhs_components, " \t");

      if (lhs_components.size() == 1) {
        ParseResult assignment_variable(Variable, lhs_components[0]);
        result.children.push_back(assignment_variable);
      } else {
            std::cerr << "Line " << this->line_number << ": Invalid syntax: '"
                   << lhs << " = ...'. Use 'variable = ...' instead."
                   << std::endl;
      }
    }

    /**
     * Parse right hand side.
     */
    DLOG(INFO) << "Parsing right hand side." << std::endl;
    Lexer::clean_string(rhs);
    ParseResult rhs_result;
    parse_string(rhs, rhs_result);

    result.children.push_back(rhs_result);

  } else {
    result.type = Error;
        std::cerr << "Line " << this->line_number
               << ": Invalid syntax: Assignment string '" << str
               << "' did not contain '='." << std::endl;
  }
}

void Lexer::parse_initialization(const std::string &str, ParseResult &result) {

  std::string cleaned_string = str;
  Lexer::clean_string(cleaned_string);

  DLOG(INFO) << "Parsing initialization: '" << cleaned_string << "'." << std::endl;

  std::string initializer_token;
  Lexer::first_token_in_string(cleaned_string, initializer_token);

  if (!initializer_token.empty()) {

    Type type = Lexer::type_for_string(initializer_token);

    if (type == Initialization) {

      /**
       * Get the list of expected arguments.
       */
      std::unordered_map<std::string, std::vector<std::string>>::const_iterator
          position_of_arguemnts =
              Lexer::arguments_for_functions.find(initializer_token);

      if (position_of_arguemnts != Lexer::arguments_for_functions.end()) {
        result.type = Initialization;
        result.value = cleaned_string;

        /**
         * Find the string representing the parameter list.
         */
        int position_of_opening_bracket = cleaned_string.find("(");

        if (position_of_opening_bracket != (int)std::string::npos &&
            position_of_opening_bracket != (int)cleaned_string.length() - 1) {
          /**
           * Find the matching closing bracket.
           */
          int position_of_closing_bracket =
              Lexer::position_of_matching_bracket_for_position(
                  cleaned_string, position_of_opening_bracket);

          if (position_of_closing_bracket == (int)cleaned_string.length() - 1) {

            /**
             * Get the argument list string.
             */
            std::string argument_list = cleaned_string.substr(
                position_of_opening_bracket + 1,
                position_of_closing_bracket - (position_of_opening_bracket + 1));

            ParseResult argument_parse_result;
            Lexer::parse_argument_list(argument_list,
                                       position_of_arguemnts->second,
                                       argument_parse_result);

            result.children.push_back(argument_parse_result);

            /**
             * If something went wrong, print usage of this
             * initializer.
             */
            if (argument_parse_result.type == Error) {
              std::cerr << "Usage of '" << initializer_token
                        << "':" << std::endl;
              std::cerr << initializer_token << "(";
              Lexer::print_argument_list(position_of_arguemnts->second);
              std::cerr << ")" << std::endl;
            }

          } else {
            /**
             * If the matching closing bracket is not at the correct
             * position, an error occurred.
             */
            result.type = Error;

            if (position_of_closing_bracket == (int)std::string::npos) {
              std::cerr << "Line " << this->line_number
                        << ": Missing closing bracket ')' ?: '"
                        << cleaned_string << "'." << std::endl;
            } else {
              std::cerr << "Line " << this->line_number
                        << ": Invalid Syntax: Unexpected statement after "
                           "bracket closing function: '"
                        << cleaned_string << "'." << std::endl;
            }
          }

        } else {
          std::cerr << "Line: " << this->line_number << ": Invalid syntax: '"
                    << cleaned_string << "'. Use 'Initializer(...)' instead."
                    << std::endl;
        }
      }
    } else {
      std::cerr << "Line " << this->line_number << ": Invalid initialization: '"
                << initializer_token
                << "' cannot be used to initialize a variable." << std::endl;
    }

  } else {
    std::cerr << "Line " << this->line_number << ": Unknown initializer: '"
              << initializer_token << "'." << std::endl;
  }
}

void Lexer::parse_number(const std::string &str, ParseResult &result) {
  DLOG(INFO) << "Parsing number: '" << str << "'." << std::endl;

  std::string cleaned_string = str;
  Lexer::clean_string(cleaned_string);

  if (Lexer::type_for_string(cleaned_string) == Number) {
    result.type = Number;
    result.value = str;
    result.children = {};
  } else {
    result.type = Error;
    std::cerr << "Line " << this->line_number << ": Invalid argument: '" << str
               << "' could not be read as '" << Lexer::name_for_type.at(Number)
               << "'." << std::endl;
  }
}

void Lexer::parse_function(const std::string &str, ParseResult &result) {
  DLOG(INFO) << "Parsing function: '" << str << "'." << std::endl;

  std::string cleaned_string = str;
  Lexer::clean_string(cleaned_string);

  /**
   * A function call consists of the function name followed by a
   * '('.  We first look for the '('.
   */
  std::size_t position_of_opening_bracket = cleaned_string.find("(");

  if (position_of_opening_bracket != std::string::npos) {
    std::string function_name =
        cleaned_string.substr(0, position_of_opening_bracket);

    /**
     * Get the arguments for the function.
     */
    std::unordered_map<std::string, std::vector<std::string>>::const_iterator
        position_of_function_arguments =
            Lexer::arguments_for_functions.find(function_name);

    if (position_of_function_arguments !=
        Lexer::arguments_for_functions.end()) {

      /**
       * Find the matching closing bracket.
       */
      int position_of_closing_bracket =
          Lexer::position_of_matching_bracket_for_position(
              cleaned_string, position_of_opening_bracket);

      /**
       * Check if the matching closing bracket was the last
       * character in the string.
       */
      if (position_of_closing_bracket == (int)cleaned_string.length() - 1) {
        std::string argument_list = cleaned_string.substr(
            position_of_opening_bracket,
            position_of_closing_bracket - position_of_opening_bracket);

        /**
         * Parse the functions argument list
         */
        ParseResult argument_parse_result;
        parse_argument_list(argument_list,
                            position_of_function_arguments->second,
                            argument_parse_result);
        result.children.push_back(argument_parse_result);
      } else {
        /**
         * If the matching closing bracket is not at the correct
         * position, an error occurred.
         */
        result.type = Error;

        if (position_of_closing_bracket == (int)std::string::npos) {
              std::cerr << "Line " << this->line_number
                     << ": Missing closing bracket ')' ?: '" << cleaned_string
                     << "'." << std::endl;
        } else {
              std::cerr << "Line " << this->line_number
                     << ": Invalid Syntax: Unexpected statement after bracket "
                        "closing function: '"
                     << cleaned_string << "'." << std::endl;
        }
      }

    } else {
      /**
       * We don't know the function.
       */
      result.type = Error;
          std::cerr << "Line " << this->line_number << ": Unknown function: '"
                 << function_name << "'." << std::endl;
    }
  } else {
    result.type = Error;
        std::cerr << "Line " << this->line_number
               << ": Invalid syntax: function call '" << str
               << "' did not contain '('. Call functions as 'function(...)."
               << std::endl;
  }
}

void Lexer::parse_argument_list(
    const std::string &str, const std::vector<std::string> &expected_arguments,
    ParseResult &result) {

  DLOG(INFO) << "Parsing argument list: '" << str << "'." << std::endl;

  result.type = ArgumentList;
  result.value = str;

  /**
   * Get the actual argument list.
   */
  std::vector<std::string> arguments;
  Lexer::components_in_string(str, arguments, ",");

  if (arguments.size() == expected_arguments.size()) {

    /**
     * Iterate the arguments and check whether the arguments match.
     */
    for (int i = 0; i < (int)arguments.size(); ++i) {

      std::string argument = arguments[i];
      Lexer::clean_string(argument);

      /**
       * Find the colon ':' that separates argument name from value.
       */
      std::size_t position_of_colon = argument.find(":");
      if (position_of_colon != std::string::npos) {

        /**
         * If the colon is the last char in the argument, something is
         * wrong.
         */
        if (position_of_colon != argument.length() - 1) {
          std::string argument_name = argument.substr(0, position_of_colon);
          Lexer::clean_string(argument_name);
          std::string argument_value =
              argument.substr(position_of_colon + 1, std::string::npos);
          Lexer::clean_string(argument_value);

          /**
           * Check whether arguments name or value are empty.
           */
          if (!argument_name.empty()) {
            if (!argument_value.empty()) {

              /**
               * Check whether the argument name matches the expected
               * one.
               */
              if (argument_name == expected_arguments[i]) {

                /**
                 * Generate the argument parse result.
                 */
                ParseResult argument_result(Argument, argument_value);

                DLOG(INFO) << "Parsing argument value: '" << argument_value
                           << "' for name: '" << argument_name << "'."
                           << std::endl;

                /**
                 * Parse the argument value.
                 */
                ParseResult argument_evaluation;
                parse_string(argument_value, argument_evaluation);
                argument_result.children.push_back(argument_evaluation);

                result.children.push_back(argument_result);
              } else {
                    std::cerr << "Line " << this->line_number
                           << ": Unexpected argument: in '" << argument
                           << "'. Expected arguments:" << std::endl;
                Lexer::print_argument_list(expected_arguments);
                std::cerr << std::endl;
              }

            } else {
              result.type = Error;
                  std::cerr << "Line " << this->line_number
                         << ": Missing argument name: in '" << argument
                         << "'. Use 'function(argument: value)' instead."
                         << std::endl;
            }
          } else {
            result.type = Error;
                std::cerr << "Line " << this->line_number
                       << ": Missing argument name: in '" << argument
                       << "'. Use 'function(argument: value)' instead."
                       << std::endl;
          }
        } else {
          result.type = Error;
              std::cerr << "Line " << this->line_number
                     << ": Missing argument value: in '" << argument
                     << "'. Use 'function(argument: value)' instead."
                     << std::endl;
        }

      } else {
        result.type = Error;
            std::cerr << "Line " << this->line_number
                   << ": Invalid syntax: Missing ':' in '" << argument
                   << "'. Use 'function(argument: value)' instead."
                   << std::endl;
      }
    }

  } else {
    result.type = Error;
        std::cerr << "Line " << this->line_number
               << ": Invalid number of parameters: '" << str
               << "'. Expected arguments:" << std::endl;
    Lexer::print_argument_list(expected_arguments);
    std::cerr << std::endl;
  }
}

void Lexer::print_argument_list(const std::vector<std::string> &arguments) {
  for (int i = 0; i < (int)arguments.size(); ++i) {
    std::cerr << arguments[i];

    if (i < (int)arguments.size() - 1) {
      std::cerr << ":";
    }
  }
}

void Lexer::print_parse_result(const ParseResult &result,
                               const std::string &indentation) {
  std::cout << indentation << Lexer::name_for_type.at(result.type) << ": "
            << result.value << std::endl;
  for (const ParseResult &child_result : result.children) {
    Lexer::print_parse_result(child_result, indentation + "\t");
  }
}
}  // namespace hydra
