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

Interpreter::Interpreter(System &system) : system(system) {}

bool Interpreter::value_for_variable(const std::string &variable,
                                     std::any &value) {
  /**
   * Reset the value to ensure that the has_value check fails when we
   * don't find a value.
   */
  value.reset();

  DLOG(INFO) << "Looking for variable '" << variable << "' in all scopes."
             << std::endl;

  for (int i = this->system.state.scopes.size() - 1; i >= 0; --i) {
    std::unordered_map<std::string, std::any>::const_iterator
      position_of_variable = this->system.state.scopes[i].find(variable);

    if (position_of_variable != this->system.state.scopes[i].end()) {
      value = position_of_variable->second;
      DLOG(INFO) << "Did find variable '" << variable << "' in scope " << i
                 << "." << std::endl;
      return true;
    }
  }

  return false;
}

void Interpreter::value_for_variable_in_current_scope(
    const std::string &variable, std::any &value) {
  DLOG(INFO) << "Looking for variable '" << variable
            << "' in current scope. (There are "
            << this->system.state.scopes.size() << " scopes)." << std::endl;

  std::unordered_map<std::string, std::any>::const_iterator
    position_of_variable = this->system.state.scopes.back().find(variable);

  if (position_of_variable != this->system.state.scopes.back().end()) {
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
    case Unknown:
      /**
       * If the type is unknown we assume that its a variable. This
       * only works, if the input has no children.
       */
      if (input.children.empty()) {
        return interpret_variable(input, result);
      } else {
        return false;
      }
    default:
      return false;
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
    this->system.print_error_message(
        std::string("Interpretation failed: An error occurred while parsing."));
    Lexer::print_parse_result(input);
    return false;
  }

  /**
   * There are two versions of assignments.  The first uses the 'var'
   * keyword and assigns a value for the first time.  The second
   * version does not use 'var' and overwrites the value of an already
   * defined variable.
   */

  /**
   * Version 1 (with 'var'):
   */

  /**
   * If the first parse result is not an Assignment, it must be a
   * variable.  If it is an Assignment, we process the assignment.
   */
  if (input.children[0].type == Assignment) {
    /**
     * A new assignment consists of exactly 3 values:
     * var variable (=) value
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
                  this->system.state.scopes.back()[input.children[1].value] =
                      value_interpretation_result;
                  result = value_interpretation_result;
                  return true;
                }
              }
            } else {
              this->system.print_error_message(
                  std::string("Redeclaration of : '") +
                  input.children[1].value + "'.");
              return false;
            }
          } else {
            this->system.print_error_message(
                std::string("Invalid assignment. Variables starting with '_' "
                            "cannot be assigned to."));
            return false;
          }

        } else {
          this->system.print_error_message(
              std::string("Invalid assignment: The variable name must not be "
                          "empty. Use 'var a = 5.0' instead."));
          return false;
        }
      } else {
        this->system.print_error_message(
            std::string("Invalid assignment. Use 'var a = 5.0' instead."));
        return false;
      }
    } else {
      this->system.print_error_message(
          std::string("Invalid assignment. Use 'var a = 5.0' instead."));
      return false;
    }
  } else {

    /**
     * Version 2 (without 'var'):
     */

    /**
     * The assignment should consist of exactly two values: the name
     * of the variable and whatever is being assigned to it.
     */
    if (input.children.size() == 2) {

      /**
       * We now check whether the variable is already defined.
       */
      std::any current_value;
      this->value_for_variable(input.children[0].value, current_value);

      if (current_value.has_value()) {
        /**
         * The variable was declared before. Now we have to interpret
         * what is being assigned.
         */
        std::any value_interpretation_result;

        /**
         * Check whether the value was interpreted successfully.
         */
        if (interpret_parse_result(input.children[1],
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
            this->system.state.scopes.back()[input.children[0].value] =
                value_interpretation_result;
            result = value_interpretation_result;
            return true;
          }
        }
      } else {
        this->system.print_error_message(
            std::string(
                "Trying to assign to undefined variable. Define the variable "
                "first using 'var ") +
            input.children[0].value + " = ...' instead.");
        return false;
      }

    } else {
      this->system.print_error_message(
          std::string("Invalid assignment. Use 'var a = 5.0' instead."));
      return false;
    }
  }

  return false;
}

bool Interpreter::interpret_initialization(const ParseResult &input,
                                           std::any &result) {
  DLOG(INFO) << "Interpreting initialization with value: '" << input.value
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  return true;
}

bool Interpreter::interpret_number(const ParseResult &input, std::any &result) {

  DLOG(INFO) << "Interpreting number with value: '" << input.value
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  try {
    double value = stod(input.value);
    result = value;
    return true;
  } catch (const std::invalid_argument &ia) {
    this->system.print_error_message(
        std::string("Interpretation failed: Invalid argument: ") + ia.what());
  }

  return false;
}

bool Interpreter::interpret_variable(const ParseResult &input,
                                     std::any &result) {
  DLOG(INFO) << "Interpreting number with value: '" << input.value
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * We now check whether the variable is defined.
   */
  bool success = this->value_for_variable(input.value, result);

  if (!success) {
    this->system.print_error_message(
        std::string("Trying to access undeclared variable '") + input.value +
        "'. Declare the variable first using 'var " + input.value + " = ...");
  }

  return success;
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
  for (int i = this->system.state.scopes.size() - 1; i >= 0; --i) {
    this->print_scope_at_index(i);
  }
}

void Interpreter::print_scope_at_index(int index) {
  std::cout << "Scope " << index << ": (" << this->system.state.scopes[index].size()
            << " variables)" << std::endl;

  /**
   * Get all keys in order to sort them afterwards.
   */
  std::vector<std::string> keys;

  for (const std::pair<std::string, std::any> &key_value :
       this->system.state.scopes[index]) {
    keys.push_back(key_value.first);
  }

  /**
   * Sort keys.
   */
  std::sort(keys.begin(), keys.end());

  for (int i = 0; i < (int)keys.size(); ++i) {
    std::cout << "  [" << i << "] " << keys[i] << " = '";
    Interpreter::print_interpretation_result(this->system.state.scopes[index].at(keys[i]));
    std::cout << "'" << std::endl;
  }
}

}  // namespace hydra
