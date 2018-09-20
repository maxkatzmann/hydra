//
//  interpreter.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#include <interpreter.hpp>
#include <lexer.hpp>

#include <iostream>

namespace hydra {

Interpreter::Interpreter() {
  this->scopes.push_back(std::unordered_map<std::string, std::any>());
}

void Interpreter::value_for_variable(const std::string &variable,
                                     std::any &value) {
  /**
   * Reset the value to ensure that the has_value check fails when we
   * don't find a value.
   */
  value.reset();

  DLOG(INFO) << "Looking for variable '" << variable << "' in all scopes."
             << std::endl;

  for (int i = this->scopes.size(); i > 0; --i) {
    std::unordered_map<std::string, std::any>::const_iterator
        position_of_variable = scopes[i].find(variable);

    if (position_of_variable != scopes[i].end()) {
      value = position_of_variable->second;
      DLOG(INFO) << "Did find variable '" << variable << "' in scope " << i
                 << "." << std::endl;
      break;
    }
  }
}

void Interpreter::value_for_variable_in_current_scope(
    const std::string &variable, std::any &value) {
  DLOG(INFO) << "Looking for variable '" << variable << "' in current scope."
             << std::endl;

  std::unordered_map<std::string, std::any>::const_iterator
    position_of_variable = this->scopes.back().find(variable);

  if (position_of_variable != this->scopes.back().end()) {
    value = position_of_variable->second;
    DLOG(INFO) << "Did find variable '" << variable << "' in current scope."
               << std::endl;
  }
}

bool Interpreter::parse_result_is_valid(const ParseResult &result) {

  if (result.type == Error) {
    return false;
  }

  for (const ParseResult &child_result : result.children) {
    if (!parse_result_is_valid(child_result)) {
      return false;
    }
  }

  return true;
}

bool Interpreter::interpret_parse_result(const ParseResult &input,
                                         std::any &result) {
  DLOG(INFO) << "Interpreting parse result with value: '" << input.value
             << "'." << std::endl;

  switch (input.type) {
    case Assignment:
      return interpret_assignment(input, result);
    case Initialization:
      return interpret_initialization(input, result);
  case Number:
    return interpret_number(input, result);
    default:
      return false;
      break;
  }
}

bool Interpreter::interpret_assignment(const ParseResult &input,
                                       std::any &result) {
  DLOG(INFO) << "Interpreting assignment with value: '" << input.value << "'."
             << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  if (!parse_result_is_valid(input)) {
    std::cerr << "Interpretation failed: An error occurred while parsing '"
              << input.value << "':" << std::endl;
    this->lexer.print_parse_result(input);
    return false;
  }

  /**
   * There are two versions of assignments.  The first uses the 'let'
   * keyword and assigns a value for the first time.  The second
   * version does not use 'let' and overwrites the value of an already
   * defined variable.
   */

  /**
   * Version 1 (with 'let'):
   */

  /**
   * If the first parse result is not an Assignment, it must be a
   * variable.  If it is an Assignment, we process the assignment.
   */
  if (input.children[0].type == Assignment) {
    /**
     * A new assignment consists of exactly 3 values:
     * let variable (=) value
     */
    if (input.children.size() == 3) {
      /**
       * The second parse result must be a variable name.
       */
      if (input.children[1].type == Variable) {

        /**
         * The variable name must not be empty.
         */
        if (!input.children[1].value.empty()) {
          /**
           * Variable assignments must not start with an
           * '_'. Underscores are reserved for internal variables.
           */
          if (input.children[1].value[0] != '_') {
            /**
             * First check whether the variable is not yet written.
             */
            std::any already_defined_value;
            value_for_variable_in_current_scope(input.children[1].value,
                                                already_defined_value);

            /**
             * Make sure the variable is not defined already.
             */
            if (!already_defined_value.has_value()) {
              /**
               * The value is not defined yet. Now we have to interpret
               * what is being assigned.
               */
              std::any value_interpretation_result;

              /**
               * Check whether the value was interpreted successfully.
               */
              if (interpret_parse_result(input.children[2],
                                         value_interpretation_result)) {
                DLOG(INFO) << "Assignment value interpreted successfully."
                           << std::endl;

                /**
                 * Check whether we actually got a value from the
                 * interpretation.
                 */
                if (value_interpretation_result.has_value()) {
                  /**
                   * Finish the assignment by adding the variable to the
                   * current scope.
                   */
                  this->scopes.back()[input.children[1].value] =
                      value_interpretation_result;
                  result = value_interpretation_result;
                  return true;
                }
              }
            } else {
              std::cerr << "Line: " << this->lexer.line_number
                        << ": Redeclaration of : '" << input.children[1].value
                        << "'." << std::endl;
              return false;
            }
          } else {
            std::cerr << "Line: " << this->lexer.line_number
                      << ": Invalid assignment: '" << input.value
                      << "'. Variables starting with '_' cannot be assigned to."
                      << std::endl;
            return false;
          }

        } else {
          std::cerr << "Line: " << this->lexer.line_number
                    << ": Invalid assignment: The variable name must not be "
                       "empty. Use 'let a = 5.0' instead."
                    << std::endl;
          return false;
        }
      } else {
        std::cerr << "Line: " << this->lexer.line_number
                  << ": Invalid assignment: '" << input.value
                  << "'. Use 'let a = 5.0' instead." << std::endl;
        return false;
      }
    } else {
      std::cerr << "Line: " << this->lexer.line_number
                << ": Invalid assignment: '" << input.value
                << "'. Use 'let a = 5.0' instead." << std::endl;
      return false;
    }
  }

  /**
   * Version 2 (without 'let'):
   */

  return false;
}

bool Interpreter::interpret_initialization(const ParseResult &input,
                                           std::any &result) {
  DLOG(INFO) << "Interpreting initialization with value: '" << input.value
             << "'." << std::endl;
  return true;
}

bool Interpreter::interpret_number(const ParseResult &input, std::any &result) {
  DLOG(INFO) << "Interpreting number with value: '" << input.value
             << "'." << std::endl;

  try {
    double value = stod(input.value);
    result = value;
    return true;
  } catch (const std::invalid_argument &ia) {
    std::cerr << "Line: " << this->lexer.line_number
              << ": Interpretation failed: Invalid argument: " << ia.what()
              << std::endl;
  }

  return false;
}

bool Interpreter::print_interpretation_result(const std::any &result) {

  try {
    std::cout << std::any_cast<double>(result);
    return true;
  } catch (const std::bad_any_cast& e) {} // Don't do anything when
                                          // the cast failed.

  return false;
}

void Interpreter::print_scopes() {
  for (int i = this->scopes.size() - 1; i >= 0; --i) {
    this->print_scope_at_index(i);
  }
}

void Interpreter::print_scope_at_index(int index) {
  std::cout << "Scope " << index << ": (" << this->scopes[index].size()
            << " variables)" << std::endl;

  /**
   * Get all keys in order to sort them afterwards.
   */
  std::vector<std::string> keys;

  for (const std::pair<std::string, std::any> &key_value :
       this->scopes[index]) {
    keys.push_back(key_value.first);
  }

  /**
   * Sort keys.
   */
  std::sort(keys.begin(), keys.end());

  for (int i = 0; i < (int)keys.size(); ++i) {
    std::cout << "  [" << i << "] " << keys[i] << " = '";
    Interpreter::print_interpretation_result(this->scopes[index].at(keys[i]));
    std::cout << "'" << std::endl;
  }
}

}  // namespace hydra
