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

Lexer::Lexer(System &system) : system(system) {

  this->known_parsers = {
      {Assignment, &Lexer::parse_assignment},
      {Expression, &Lexer::parse_expression},
      {Loop, &Lexer::parse_loop},
      {Function, &Lexer::parse_function},
      {Initialization, &Lexer::parse_initialization},
      {Number, &Lexer::parse_number},
      {Parenthesis, &Lexer::parse_parenthesis},
      {Range, &Lexer::parse_range},
      {String, &Lexer::parse_string_token}
  };
}

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

bool Lexer::is_string_empty(std::string &str) {
  return str.empty() || str.find_first_not_of(" \t", 0) == std::string::npos;
}

int Lexer::position_of_matching_bracket_for_position(const std::string &str,
                                                     const char opening_bracket,
                                                     int position) {
  const std::unordered_map<char, char> bracket_pairs = {{'(', ')'}, {'[', ']'}, {'{', '}'}, {'"', '"'}};

  const char closing_bracket = bracket_pairs.at(opening_bracket);

  /**
   * Before parsing the function arguments, find the closing
   * bracket.
   */
  int position_of_closing_bracket = -1;

  /**
   * The first bracket is already open, since is the one we just
   * found.
   */
  int number_of_open_brackets = 1;

  /**
   * Iterate the characters in the string to find the matching closing
   * bracket. Obviously, we don't need to check the position itself,
   * so we start at the next position instead.
   */
  for (int string_index = position + 1; string_index < (int)str.length();
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

int Lexer::position_of_matching_quote_for_position(const std::string &str,
                                                     int position) {

  /**
   * Iterate the characters in the string to find the matching closing
   * bracket. Obviously, we don't need to check the position itself,
   * so we start at the next position instead.
   */
  for (int string_index = position + 1; string_index < (int)str.length();
       ++string_index) {

    /**
     * Dealing open brackets.
     */
    if (str[string_index] == '"') {
      return string_index;
    }
  }

  return (int)std::string::npos;
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
          cleaned_string, cleaned_string[current_index], current_index);

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
       * We already tokenized the contents of in the bracket, so we
       * now continue with the contents after the bracket.
       */
      current_index = matching_bracket + 1;

    } else if (cleaned_string[current_index] == '"') {
      /**
       * If we're dealing with a string, we identify the whole string
       * and save it as token.
       */
      int matching_quote = Lexer::position_of_matching_quote_for_position(
          cleaned_string, current_index);

      std::string string_content = cleaned_string.substr(
          current_index + 1, matching_quote - (current_index + 1));

      /**
       * Creating the token.
       */
      Token string_token(string_content, String);

      /**
       * Now we check whether there are escapes within the string.
       *
       * For this we keep track of where the last escape ended. So
       * that we can add the content in between as normal string
       * content.
       *
       * Since we start to gather the string right after the position
       * of the last escape (+1) but we initially start want to start
       * our string at 0, we have -1 here.
       */
      int position_of_last_escape_end = -1;
      int position_of_escape = string_content.find_first_of("\\");

      /**
       * As long as we find an escape, we identify the part within the
       * escape and parse these contents.
       */
      while (position_of_escape != (int)std::string::npos) {

        /**
         * If we're already to far at the end, we can stop looking for
         * escape sequences.
         */
        if (position_of_escape >= (int)string_content.length() - 1) {
          break;
        }

        /**
         * If the character after the \ is not a ( we have not really
         * found the escape sequence.
         */
        if (string_content[position_of_escape + 1] != '(') {

          /**
           * We did not really found an escape marker, so we continue
           * searching for the next possible position.
           */
          position_of_escape =
              string_content.find_first_of("\\", position_of_escape + 1);
          continue;
        }

        /**
         * We found the \( escape marker but the current
         * position_of_escape is on \. So we add 1.
         */
        ++position_of_escape;

        DLOG(INFO) << "Found escape at position: " << position_of_escape
                   << std::endl;
        /**
         * If we found an escape character, we add the part of the
         * string before the escape as a child of the string token.
         */
        std::string previous_string_part = string_content.substr(
            position_of_last_escape_end + 1,
            position_of_escape - 1 - (position_of_last_escape_end + 1));

        /**
         * Add the previous part of the token if its not empty.
         */
        if (!previous_string_part.empty()) {
          Token previous_part_token(previous_string_part, String);
          string_token.children.push_back(previous_part_token);
        }

        /**
         * Find the parenthesis closing this escape marker.
         */
        int position_of_matching_bracket =
            position_of_matching_bracket_for_position(string_content, '(',
                                                      position_of_escape);

        DLOG(INFO) << "Position of escape closing bracket: "
                   << position_of_matching_bracket << std::endl;

        /**
         * Check whether the matching bracket was found.
         */
        if (position_of_matching_bracket == (int)std::string::npos) {
          this->system.print_error_message(
              std::string("Invalid syntax. Could not find matching bracket for "
                          "'\\(' at character index ") +
              std::to_string(position_of_escape) + ".");
          return false;
        }

        /**
         * Get the string representing the content within the escape.
         */
        std::string escaped_content = string_content.substr(
            position_of_escape + 1,
            position_of_matching_bracket - (position_of_escape + 1));

        DLOG(INFO) << "Found escaped content: '" << escaped_content << "'"
                   << std::endl;

        /**
         * We now create a new string escape token that becomes a
         * child from the string token.
         *
         * Then we tokenize the string within this escape part into
         * the children of that escaped_content token.
         */
        Token escape_token(escaped_content, StringEscape);
        tokenize_string(escaped_content, escape_token.children);

        /**
         * Add the escape_token as child of the string_token.
         */
        string_token.children.push_back(escape_token);

        /**
         * If we're at the end of the string we can just stop looking
         * for escape markers now.
         */
        if (position_of_matching_bracket >= (int)string_content.length() - 1) {
          break;
        }

        /**
         * Keep track of where the last escape ended.
         */
        position_of_last_escape_end = position_of_matching_bracket;

        /**
         * Now we update the position_of_escape with the position of
         * the next escape marker.
         */
        position_of_escape = string_content.find_first_of(
            "\\", position_of_matching_bracket + 1);

        /**
         * If we don't have another escape part, we simply add the
         * remainder of the string as last string part.
         */
        if (position_of_escape == (int)std::string::npos) {
          /**
           * If we could not find a new escape marker, we simply add the
           * remainder of the string as child of the current
           * string_token.
           */
          std::string last_string_part =
            string_content.substr(position_of_matching_bracket + 1);

          /**
           * Add the last string part if its not empty.
           */
          if (!last_string_part.empty()) {
            Token last_part_token(last_string_part, String);
            string_token.children.push_back(last_part_token);
          }
        }
      }

      /**
       * Add the string token.
       */
      tokens.push_back(string_token);

      /**
       * We already tokenized the whole string, so we now continue
       * with the contents after the quote.
       */
      current_index = matching_quote + 1;
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
    if (!Lexer::is_string_empty(token.value) || !token.children.empty()) {
      tokens.push_back(token);
    }

  } while (current_index != (int)std::string::npos &&
           current_index <= (int)cleaned_string.length());

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

bool Lexer::parse_code(const std::vector<std::string> &code,
                       std::vector<ParseResult> &parsed_code) {
  /**
   * We iterate all lines of code and try to parse them.
   *
   * Loops, however, have a special role. When a loop is started, all
   * lines within the loop belong to the loop. That is, all the lines
   * in the loop become children of the loop parse result. Since we
   * can have nested loops, we need to keep track of where the current
   * code lines belong.
   *
   * The code_scopes stores vectors of parse results. When a line is
   * being parsed, the result is added to the last vector in the
   * code_scopes vector. If we're in a loop, this last vector is the
   * child-vector of the corresponding loop-parse-result.
   */
  std::vector<std::vector<ParseResult> *> code_scopes = {&parsed_code};

  /**
   * In order to being able to print meaningful error messages, we
   * keep track of which loop was opened in which line. Since we don't
   * have a loop initially, we assign -1 to the first scope.
   */
  std::vector<int> line_number_for_scope = {-1};

  /**
   * Iterate all lines.
   */
  for (int line_number = 0; line_number < (int)code.size(); ++line_number) {

    /**
     * Let the system know about which line we're currently dealing
     * with.
     */
    this->system.line_number = line_number;
    this->system.current_line = code[line_number];

    /**
     * Parse the line.
     */
    ParseResult line_parse_result;

    /**
     * Assign the line number to the parse result. Offset with 1 since
     * line numbers don't start at 0.
     */
    line_parse_result.line_number = line_number + 1;

    if (!parse_string(code[line_number], line_parse_result)) {
      return false;
    }

    /**
     * If the line is a loop-definition, we add the loop definition to
     * the back of the current code_scope AND open a new code scope by
     * adding the children of the loop-definition to the code_scopes.
     */
    if (line_parse_result.type == Loop) {
      code_scopes.back()->push_back(line_parse_result);
      code_scopes.push_back(&(code_scopes.back()->back().children));

      /**
       * Keep track of the line number associated with this loop.
       */
      line_number_for_scope.push_back(line_parse_result.line_number);
    } else if (line_parse_result.type == Parenthesis) {
      /**
       * If the line is of type Parenthesis, it is meant to close a
       * loop. We close the loop by removing the children vector of
       * the loop-parse-result from the code_scopes vector.
       */
      code_scopes.pop_back();
      line_number_for_scope.pop_back();
      /**
       * The parenthesis itself is useless and does not need to be
       * interpreted later, so we don't add it.
       */
    } else {
      /**
       * The line is not a loop or parenthesis so we simply add it in
       * the current code_scope.
       */
      code_scopes.back()->push_back(line_parse_result);
    }
  }

  /**
   * We check whether all opened loops have been closed. Which is the
   * case, if the code_scopes vector contains only the initial scope.
   */
  if (code_scopes.size() != 1) {
    this->system.print_error_message(
        std::string(
            "Could not parse code. Missing parenthesis to loop in line: ") +
        std::to_string(line_number_for_scope.back()) + ".");
    return false;
  }

  /**
   * If we reach this point, everything went well. Reset the state
   * values.
   */
  this->system.state.line_number = -1;
  this->system.state.current_line = "";
  return true;
}

bool Lexer::parse_string(const std::string &str, ParseResult &result) {

  DLOG(INFO) << "Parsing string: '" << str << "'." << std::endl;

  /**
   * Tokenize the string.
   */
  std::vector<Token> tokens;
  if (tokenize_string(str, tokens)) {
#ifdef DEBUG
    Lexer::print_tokenized_string(tokens);
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

  if (type == Empty) {
    result.type = Empty;
    return true;
  }

  /**
   * Find the parser that should be used for a tokenized string of
   * that type.
   */
  std::unordered_map<Type,
                     std::function<bool(Lexer *, const std::vector<Token> &,
                                        ParseResult &)>>::const_iterator
      position_of_parser = this->known_parsers.find(type);

  if (position_of_parser != this->known_parsers.end()) {
    /**
     * Call the found parser function.
     */
    return position_of_parser->second(this, tokens, result);
  } else {
    /**
     * We don't know what to do with that. Maybe its a variable name?
     * If we're dealing with a single token this might be the
     * case. Otherwise something weird happened and we return an
     * error.
     */
    if (tokens.size() == 1) {
      DLOG(INFO) << "Unknown token: '" << tokens[0].value
                 << "'. Maybe it is a variable name..." << std::endl;
      result.type = Unknown;
      result.value = tokens[0].value;
      return true;
    } else {
      result.type = Error;
      this->system.print_error_message(
          std::string("Could not parse line. Type of statement unclear."));
      return false;
    }
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
      assignment_variable.line_number = result.line_number;
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
    if (lhs[0].value != "var") {
      result.type = Error;
      this->system.print_error_message(std::string(
          "Invalid assignment. Use 'var a = 10.0' to declare a variable."));
      return false;
    }

    /**
     * There are exactly two tokens. The first is 'var'. Now the
     * second has to be Unknown since it should be a variable name.
     */
    if (lhs[1].type != Unknown) {
      result.type = Error;
      this->system.print_error_message(
          std::string(
              "Invalid assignment. Expected variable name but found '") +
          lhs[0].value +
          " instead. Use 'a = 10.0' or 'var a = 10.0' to assign a variable.");
      return false;
    }

    /**
     * Everything went as expected.
     */
    ParseResult assignment_keyword(Assignment, lhs[0].value);
    assignment_keyword.line_number = result.line_number;
    ParseResult assignment_variable(Variable, lhs[1].value);
    assignment_variable.line_number = result.line_number;

    result.children.push_back(assignment_keyword);
    result.children.push_back(assignment_variable);

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
  rhs_result.line_number = result.line_number;
  bool success = parse_tokens(rhs, rhs_result);
  result.children.push_back(rhs_result);

  if (!success) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Error while parsing right hand side of assignment."));
  }
  return success;
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
        if (tokens[index].type == Operator) {
          result.type = Error;
          this->system.print_error_message(
              std::string("Invalid syntax: Unexpectedly found operator."));
          return false;
        }
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
        /**
         * Odd indices are supposed to be operators.
         */
        if (tokens[index].type != Operator) {
          result.type = Error;
          this->system.print_error_message(
              std::string("Invalid syntax: Expected operator but found '") +
              tokens[index].value + "' instead.");
          return false;
        }
        /**
         * We found an operator.
         */
        ParseResult operator_result(Operator, tokens[index].value);
        result.children.push_back(operator_result);
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
  if (tokens.size() != 1) {
    result.type = Error;
    this->system.print_error_message(std::string(
        "Invalid number of statements. Use only one function call per line."));
    return false;
  }

  /**
   * The first token should be a function.
   */
  if (tokens[0].type != Function) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid syntax. Expected function name but found '") +
        tokens[0].value + "', which is of type '" +
        System::name_for_type.at(tokens[0].type));
    return false;
  }

  result.type = Function;
  result.value = tokens[0].value;

  /**
   * Get the arguments for the function.
   */
  std::unordered_map<std::string, Func>::const_iterator
      position_of_function_arguments =
          this->system.known_functions.find(tokens[0].value);

  /**
   * Check whether we know this function.
   */
  if (position_of_function_arguments != this->system.known_functions.end()) {
    /**
     * Parse the functions argument list
     */
    ParseResult argument_parse_result;
    argument_parse_result.line_number = result.line_number; // Pass on line_number.
    bool success = parse_argument_list(
        tokens[0].children, position_of_function_arguments->second.arguments,
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
      std::cerr << "> Usage of '" << tokens[0].value << "': " << tokens[0].value
                << "(";
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

  /**
   * If we didn't return, yet, something went wrong.
   */
  return false;
}

bool Lexer::parse_initialization(const std::vector<Token> &tokens,
                                 ParseResult &result) {

  /**
   * Check whether the first token signals an initialization.
   */
  if (tokens[0].type != Initialization) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid initialization: '") + tokens[0].value +
        "' cannot be used to initialize a variable.");
    return false;
  }

  /**
   * The type is initialization.
   */
  result.type = Initialization;
  result.value = tokens[0].value;

  /**
   * Get the list of expected arguments.
   */
  std::unordered_map<std::string, Func>::const_iterator position_of_arguments =
      this->system.known_functions.find(tokens[0].value);

  if (position_of_arguments != this->system.known_functions.end()) {
    /**
     * The arguments of an initialization are stored as children
     * of the initialization token.
     *
     * They must not be empty.
     */
    if (tokens[0].children.empty()) {
      /**
       * If the argument list was empty, print an error.
       */
      result.type = Error;

      this->system.print_error_message(
          std::string("Missing arguments during initialization of '") +
          tokens[0].value + "'.");
      std::cerr << "> Usage of '" << tokens[0].value << "': " << tokens[0].value
                << "(";
      System::print_argument_list(position_of_arguments->second.arguments);
      std::cerr << ")" << std::endl;
      return false;
    }

    ParseResult argument_parse_result;
    bool success = Lexer::parse_argument_list(
        tokens[0].children, position_of_arguments->second.arguments,
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
      std::cerr << "> Usage of '" << tokens[0].value << "': " << tokens[0].value
                << "(";
      System::print_argument_list(position_of_arguments->second.arguments);
      std::cerr << ")" << std::endl;
    }

    return success;
  }

  /**
   * If we haven't returned, yet, something went wrong.
   */
  return false;
}

bool Lexer::parse_loop(const std::vector<Token> &tokens,
                           ParseResult &result) {
  /**
   * A for loop consists of
   *
   * the keyword 'for'
   * followed by the loop-variable name
   * followed by the keyword 'in'
   * then the range [...]
   * and finally the opening parenthesis.
   *
   * which makes 5 tokens in total.
   */
  if (tokens.size() != 5) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Could not parse for-loop: invalid number of argument."));
    return false;
  }

  /**
   * The first token must be 'for'
   */
  if (tokens[0].value != "for") {
    result.type = Error;
    this->system.print_error_message(
        std::string("Could not parse for-loop: invalid number of argument."));
    return false;
  }

  result.type = Loop;
  result.value = tokens[0].value;

  /**
   * Now we get the loop variable. Since the lexer doesn't know what
   * variables are, Unknown may also represent a variable.
   */
  if (tokens[1].type != Unknown && tokens[1].type != Variable) {
    result.type = Error;
    this->system.print_error_message(
        std::string(
            "Could not parse for-loop: Expected variable name but found '") +
        System::name_for_type.at(tokens[1].type) + "'.");
    return false;
  }

  /**
   * The loop variable is the first child of the
   * for-loop-parse-result.
   */
  ParseResult variable_parse_result = ParseResult(Variable, tokens[1].value);
  variable_parse_result.line_number = result.line_number; // Pass the line_number.
  result.children.push_back(variable_parse_result);

  /**
   * We now expect the keyword in.
   */
  if (tokens[2].value != "in") {
    result.type = Error;
    this->system.print_error_message(
        std::string("Could not parse for-loop: Expected 'in' but found '") +
        System::name_for_type.at(tokens[2].type) +
        "' after the loop variable.");
    return false;
  }

  /**
   * Next up is the range.
   */
  if (tokens[3].type != Range) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Could not parse for-loop: Expected 'Range' but found '") +
        System::name_for_type.at(tokens[3].type) + "' instead.");
    return false;
  }

  ParseResult range_parse_result;
  range_parse_result.line_number = result.line_number; // Pass the line_number.
  bool range_parse_success = parse_range({tokens[3]}, range_parse_result);

  if (!range_parse_success) {
    result.type = Error;
    return false;
  }

  /**
   * The range is the second child of the for loop.
   */
  result.children.push_back(range_parse_result);

  /**
   * Finally we check whether the last token is the opening
   * parenthesis.
   */
  if (tokens[4].value != "{") {
    result.type = Error;
    this->system.print_error_message(
        std::string("Could not parse for-loop: Expected '{' but found '") +
        tokens[4].value + "' instead.");
    return false;
  }

  return true;
}

bool Lexer::parse_number(const std::vector<Token> &tokens,
                         ParseResult &result) {

  /**
   * Check if the this is a single number.
   */
  if (tokens.size() != 1) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid number of arguments near '") + tokens[0].value +
        "'. Token could not be read as '" + System::name_for_type.at(Number) +
        "'.");
    return false;
  }

  if (tokens[0].type != Number) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid argument: '") + tokens[0].value +
        "' could not be read as '" + System::name_for_type.at(Number) + "'.");
    return false;
  }

  result.type = Number;
  result.value = tokens[0].value;
  return true;
}

bool Lexer::parse_parenthesis(const std::vector<Token> &tokens,
                              ParseResult &result) {
  /**
   * A line that contains a parenthesis has to have exactly one token
   * and that token is the parenthesis.
   */
  if (tokens.size() != 1) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid number of arguments. You can only have on '}' in "
                    "a line."));
    return false;
  }

  /**
   * A token representing a parenthesis has to be '}'.
   */
  if (tokens[0].value != "}") {

    result.type = Error;
    /**
     * If it is a '{' instead, maybe the opening parenthesis of a
     * for-loop was placed on a new line. Which we don't allow.
     */
    if (tokens[0].value == "{") {
      this->system.print_error_message(std::string(
          "A line must not start with '{', please place parentheses of "
          "for-loops in the same line as the loop-definition."));
    } else {
      this->system.print_error_message(
          std::string("Expected '}', but found '") + tokens[0].value +
          "' instead.");
    }

    return false;
  }

  result.type = Parenthesis;
  result.value = tokens[0].value;
  return true;
}

/**
 * Parses a string that represents a range.  If the parsed string
 * is not a number, the result will have type Error.
 */
bool Lexer::parse_range(const std::vector<Token> &tokens, ParseResult &result) {

  /**
   * A range is a single token that has the arguments of the range as
   * its children.
   */
  if (tokens.size() != 1) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid number of arguments for type '") +
        System::name_for_type.at(Range) + "'. Expected one, but found " +
        std::to_string(tokens.size()) + ".");
    return false;
  }

  /**
   * Check whether we're dealing with a range.
   */

  if (tokens[0].type != Range) {
    result.type = Error;
    this->system.print_error_message(std::string("Unexpectedly found '") +
                                     System::name_for_type.at(tokens[0].type) +
                                     "' while trying to parse a range.");
    return false;
  }

  /**
   * A range token has at least 5 children.
   *
   * 'from , step , to' (5 including commas.)
   */
  if (tokens[0].children.size() < 5) {
    result.type = Error;
    this->system.print_error_message(
        std::string(
            "Invalid number of arguments in range. Expected one, but found ") +
        std::to_string(tokens.size()) + ".");
    return false;
  }

  result.type = Range;
  result.value = tokens[0].value;

  /**
   * Now we parse the three range tokens.
   */
  std::vector<Token> tokens_in_current_range_argument;
  for (int index = 0; index <= (int)tokens[0].children.size(); ++index) {

    /**
     * If we find a comma or are at the very end of the arguments
     * vector, all previous tokens from the current argument of the
     * range. So we parse this argument.
     */
    if (index == (int)tokens[0].children.size() || tokens[0].children[index].value == ",") {
      ParseResult current_argument;
      current_argument.line_number = result.line_number;

      /**
       * Check whether we could successfully parse the current
       * argument.
       */
      if (!parse_tokens(tokens_in_current_range_argument, current_argument)) {
        return false;
      }

      /**
       * Add the argument to the range.
       */
      result.children.push_back(current_argument);

      /**
       * Reset the tokens array so we can use it for the next
       * argument.
       */
      tokens_in_current_range_argument.clear();
    } else {
      /**
       * If we didn't find a comma, we add the token to the current
       * argument.
       */
      tokens_in_current_range_argument.push_back(tokens[0].children[index]);
    }
  }

  /**
   * We now check whether we found exactly 3 arguments. If not
   * something went wrong.
   */
  if (result.children.size() != 3) {
    this->system.print_error_message(
        std::string(
            "Invalid number of arguments in range. Expected 3 but found ") +
        std::to_string(result.children.size()) + " instead.");
    return false;
  }

  return true;
}

bool Lexer::parse_string_token(const std::vector<Token> &tokens,
                               ParseResult &result) {

  /**
   * A string token has to consist of exactly one token. Which
   * contains the string.
   */
  if (tokens.size() != 1) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Invalid number of argument in string."));
    return false;
  }

  /**
   * If the type is not string we have an error.
   */
  if (tokens[0].type != String) {
    result.type = Error;
    this->system.print_error_message(std::string("Unexpectedly found '") +
                                     System::name_for_type.at(tokens[0].type) +
                                     "' while trying to parse a string.");
    return false;
  }

  /**
   * At this point we're sure that we have a string. It remains to
   * check whether there are escapes in there.
   */
  result.type = String;
  result.value = tokens[0].value;

  /**
   * If there are no children, we don't have escapes and simply pass
   * on the string value.
   */
  if (tokens[0].children.empty()) {
    return true;
  }

  /**
   * If a string contains escapes, then the actual string value is
   * irrelevant, and we have to put the string together from the child
   * tokens.
   */
  int escape_index = 0; // Used for better error messages.

  for (const Token &child : tokens[0].children) {

    if (child.type == String) {
      ParseResult string_token_parse_result;
      string_token_parse_result.line_number = result.line_number;

      /**
       * Try to parse the string.
       */
      if (!parse_string_token({child}, string_token_parse_result)) {
        result.type = Error;
        return false;
      }

      /**
       * Add the token.
       */
      result.children.push_back(string_token_parse_result);

      /**
       * If the token is not a String, it might be a StringEscape.
       */
    } else if (child.type == StringEscape) {

      /**
       * We just found a new StringEscape.
       */
      ++escape_index;

      /**
       * If the StringEscape does not have a content, something went
       * wrong.
       */
      if (child.children.empty()) {
        result.type = Error;
        this->system.print_error_message(
            std::string("Invalid syntax. Escape sequence .") +
            std::to_string(escape_index) + " is empty.");
        return false;
      }

      /**
       * Parse the contents of the StringEscape.
       */
      ParseResult string_escape_parse_result;
      string_escape_parse_result.line_number = result.line_number;

      /**
       * Try to parse the content of the escape.
       */
      if (!parse_tokens(child.children, string_escape_parse_result)) {
        result.type = Error;
        this->system.print_error_message(
            std::string("Could not parse escape sequence ") +
            std::to_string(escape_index) + ".");
        return false;
      }

      /**
       * Add the ParseResult as child of the overall result.
       */
      result.children.push_back(string_escape_parse_result);
    }
  }

  /**
   * If we reach this point without failing, everything went as
   * expected.
   */
  return true;
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
  if (tokens.empty()) {
    result.type = Error;
    this->system.print_error_message(
        std::string("Missing argument in function call."));
    return false;
  }

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
     * than are expected. We check for >= since we are currently
     * finding a new argument. If we already have enough this means
     * we're currently finding an extra argument.
     */
    if (number_of_found_arguments >= (int)expected_arguments.size()) {
      result.type = Error;
      this->system.print_error_message(
          std::string("Extraneous argument in function call."));
      return false;
    }

    /**
     * Check whether the argument matches the expected argument.
     */
    std::string argument_name = tokens[token_index].value;
    if (argument_name != expected_arguments[number_of_found_arguments]) {
      result.type = Error;
      this->system.print_error_message(
          std::string("Invalid argument in function call. Expected '") +
          expected_arguments[number_of_found_arguments] + "' but found '" +
          tokens[token_index].value + "' instead.");
      return false;
    }

    /**
     * Arguments match. Next up: check whether the next token is
     * a ':'. Thus, check if we're not already at the end of the
     * argument list.
     */
    ++token_index;
    if (token_index >= (int)tokens.size()) {
      result.type = Error;
      this->system.print_error_message(
          std::string("Missing argument value in function call."));
      return false;
    }

    /**
     * Check if this next token is a ':'.
     */
    if (tokens[token_index].value != ":") {
      result.type = Error;
      this->system.print_error_message(
          std::string("Invalid syntax in function call. Expected ':' "
                      "but found '") +
          tokens[token_index].value + "' instead.");
      return false;
    }

    /**
     * Now collect the argument values. First check again if
     * we're not at the end of the argument list.
     */
    ++token_index;

    if (token_index >= (int)tokens.size()) {
      result.type = Error;
      this->system.print_error_message(
          std::string("Missing argument value in function call."));
      return false;
    }

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
     * We have now collected all values for this argument.  Parse
     * them!
     */
    ParseResult argument_result(Argument, argument_name);
    argument_result.line_number = result.line_number;

    /**
     * Parse the argument value.
     */
    ParseResult argument_evaluation;
    argument_evaluation.line_number = result.line_number;
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
  }

  /**
   * When we arrived here, we parsed the whole argument list
   * successfully.
   */
  return true;
}

void Lexer::print_parse_result(const ParseResult &result,
                               const std::string &indentation) {
  std::cout << indentation << System::name_for_type.at(result.type) << ": '"
            << result.value << "' (" << result.line_number << ")" << std::endl;
  for (const ParseResult &child_result : result.children) {
    Lexer::print_parse_result(child_result, indentation + "\t");
  }
}

void Lexer::print_tokenized_string(const std::vector<Token> &tokenized_string,
                                   const std::string &indentation) {
  for (const Token &token : tokenized_string) {
    std::cout << indentation << "'" << token.value << "' ("
              << System::name_for_type.at(token.type) << ")" << std::endl;
    Lexer::print_tokenized_string(token.children, indentation + "\t");
  }
}
}  // namespace hydra
