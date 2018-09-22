//
//  interpreter.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#include <interpreter.hpp>
#include <lexer.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

namespace hydra {

Interpreter::Interpreter(System &system) : system(system) {

  this->builtin_functions = {
      {"cos", &Interpreter::function_cos},
      {"cosh", &Interpreter::function_cosh},
      {"exp", &Interpreter::function_exp},
      {"print", &Interpreter::function_print},
      {"sin", &Interpreter::function_sin},
      {"sinh", &Interpreter::function_sinh},
      {"random", &Interpreter::function_random}
  };

  this->known_interpretations = {
      {Assignment, &Interpreter::interpret_assignment},
      {Initialization, &Interpreter::interpret_initialization},
      {Expression, &Interpreter::interpret_expression},
      {Function, &Interpreter::interpret_function},
      {Number, &Interpreter::interpret_number},
      {String, &Interpreter::interpret_string},
      {Unknown, &Interpreter::interpret_unknown},
      {Variable, &Interpreter::interpret_variable}
  };

}

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

bool Interpreter::number_from_value(const std::any &value, double &number) {

  try {
    number = std::any_cast<double>(value);
    return true;
  } catch (std::bad_any_cast &bac) {
    return false;
  }
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
  DLOG(INFO) << "Interpreting parse result of type: '" << System::name_for_type.at(input.type)
             << "'." << std::endl;

  if (input.type == Error) {
    this->system.print_error_message(
        std::string("Cannot interpret statemet of type '") +
        System::name_for_type.at(input.type) + "'.");
    return false;
  }

  /**
   * Try to find the required interpretation method for the type of
   * this input.
   */
  std::unordered_map<Type,
                     std::function<bool(Interpreter *, const ParseResult &,
                                        std::any &)>>::const_iterator
      position_of_interpretation = this->known_interpretations.find(input.type);

  if (position_of_interpretation != this->known_interpretations.end()) {
    return position_of_interpretation->second(this, input, result);
  } else {
    this->system.print_error_message(
        std::string("Interpretation failed. No interpretation defined for "
                    "input of type '") +
        System::name_for_type.at(input.type) + "'.");
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
                  std::string("Redefinition of : '") +
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

    /**
     * The string could not be cast to double. Maybe we're dealing
     * with the constant M_PI?
     */
    if (input.value == "M_PI") {
      result = M_PI;
      return true;
    }

    /**
     * If the input is not M_PI we really have no idea what number it
     * should represent.
     */
    this->system.print_error_message(
        std::string("Interpretation failed: Invalid argument: ") + ia.what());
    return false;
  }

  return false;
}

bool Interpreter::interpret_string(const ParseResult &input, std::any &result) {
  DLOG(INFO) << "Interpreting string with value: '" << input.value << "'."
             << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  if (input.type == String) {
    result = input.value;
    return true;
  } else {
    this->system.print_error_message(
        std::string("Interpretation failed. Unexpectedly found '") +
        System::name_for_type.at(input.type) +
        "' while interpreting a string.");
    return false;
  }
}

bool Interpreter::interpret_unknown(const ParseResult &input,
                                    std::any &result) {
  /**
   * If the input type is unknown we check whether it might be a
   * variable.  (The lexer doesn't know about the defined variables.)
   */
  if (input.type == Unknown) {
    /**
     * If the type is unknown we assume that its a variable. This
     * only works, if the input has no children.
     */
    if (input.children.empty()) {
      return interpret_variable(input, result);
    } else {
      return false;
    }
  } else {
    /**
     * If the input type is not unknown something is wrong.
     */
    this->system.print_error_message(
        std::string("Interpretation failed. Unexpectedly found '") +
        System::name_for_type.at(input.type) +
        "' while interpreting something unknown.");
    return false;
  }
}

bool Interpreter::interpret_expression(const ParseResult &input,
                                       std::any &result) {

  DLOG(INFO) << "Interpreting mathematical expression." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * A mathematical expression consists of statements that evaluate to
   * number, and operators in an alternating manner.  These are stored
   * as the children of the expression ParseResult.
   */

  /**
   * Since multiplication and division come before addition and
   * subtraction we first evaluate these operators and then the rest.
   *
   * We thus iterate the parts of the expression and collect all parts
   * that are not part of a multiplication or division.  Additionally,
   * we immediately evaluate multiplications and divisions and add the
   * resulting value to the collection of expression parts that still
   * need to be evaluated.
   */
  std::vector<ParseResult> remaining_expression;

  /**
   * When we find a '*' or '/' we manipulate the index, since in that
   * case we can skip the next (it will be evaluated in the current
   * iteration.)  Since index manipulation within for-loops is bad
   * style, we use a while-loop instead.
   */
  int index = 0;
  while (index < (int)input.children.size()) {

    /**
     * If we find an operator at the current index and it's '*' or '/'
     * we immediately evaluate the corresponding operation by
     * evaluating the preceding and following statements.
     */

    /**
     * Check whether we find an operator.
     */
    if (input.children[index].type == Operator) {

      /**
       * Check if the operator is at an odd index. Otherwise something
       * is wrong.
       */
      if (index % 2 == 1) {

        /**
         * The string representing the current operator.
         */
        std::string operator_string = input.children[index].value;

        /**
         * Check whether the operator is '*' or '/'
         */
        if ("*" == operator_string || "/" == operator_string) {
          /**
           * We now evaluate the operation.  For this, we evaluate the
           * preceding and following part of the expression.
           *
           * Start with the left hand side.
           */
          std::any lhs_result;

          /**
           * We now use the last element of the REMAINING_EXPRESSION
           * as left hand side. This is due to the following
           * reason. In the previous iteration we added the previous
           * element of the expression to the end of the
           * remaining_expression vector. We now use it and then pop
           * it from the remaining_expression since we won't evaluate
           * it again.
           *
           * If the previous element was already a right hand side of
           * a multiplication or division, this right hand side was
           * already evaluated and the result of the multiplication or
           * division was added to the back of the
           * remaining_expression vector. Which means now taking this
           * element from the end is exactly what we want.
           */
          bool lhs_interpretation_success =
              interpret_parse_result(remaining_expression.back(), lhs_result);

          /**
           * We don't want to interpret this element again.
           */
          remaining_expression.pop_back();

          /**
           * Check whether the left hand side was evaluated
           * successfully.
           */
          if (!lhs_interpretation_success) {
            this->system.print_error_message(
                std::string(
                    "Could not interpret left hand side of operation near '") +
                operator_string + "'.");
            return false;
          }

          /**
           * We now try to get the numerical value of the left hand
           * side.  Operators can only be applied to numbers. So we
           * try to cast the result to a number.
           */
          double lhs_value;

          try {
            lhs_value = std::any_cast<double>(lhs_result);
          } catch (const std::bad_any_cast &bac) {
            this->system.print_error_message(
                std::string("Interpretation failed: Left hand side of "
                            "operation near '") +
                operator_string +
                "', could not be interpreted as number :" + bac.what());
            return false;
          }

          /**
           * Now proceed with the right hand side.
           */

          std::any rhs_result;
          bool rhs_interpretation_success =
              interpret_parse_result(input.children[index + 1], rhs_result);

          /**
           * Check whether the left hand side was evaluated
           * successfully.
           */
          if (!rhs_interpretation_success) {
            this->system.print_error_message(
                std::string(
                    "Could not interpret right hand side of operation near '") +
                operator_string + "'.");
            return false;
          }

          /**
           * We now try to get the numerical value of the left hand
           * side.  Operators can only be applied to numbers. So we
           * try to cast the result to a number.
           */
          double rhs_value;

          try {
            rhs_value = std::any_cast<double>(rhs_result);
          } catch (const std::bad_any_cast &bac) {
            this->system.print_error_message(
                std::string("Interpretation failed: Right hand side of "
                            "operation near '") +
                operator_string +
                "', could not be interpreted as number :" + bac.what());
            return false;
          }

          /**
           * Now we have the left hand side and the right hand
           * side. So we can actually evaluate the expression.
           */
          double result_value;

          /**
           * Perform multiplications
           */
          if ("*" == operator_string) {
            result_value = lhs_value * rhs_value;
            DLOG(INFO) << "Intermediate result of expression: " << lhs_value
                       << " * " << rhs_value << " = " << result_value;
          } else {
            result_value = lhs_value / rhs_value;

            DLOG(INFO) << "Intermediate result of expression: " << lhs_value
                       << " / " << rhs_value << " = " << result_value;
          }

          /**
           * Since we later need to evaluate the whole expression, we
           * now add this intermediate result as ParseResult so later
           * evaluation becomes easier.
           */
          ParseResult intermediate_result(Number, std::to_string(result_value));

          /**
           * We now simply add the result at the end of the
           * remaining_expression in order to be evaluated later.
           */
          remaining_expression.push_back(intermediate_result);

          /**
           * Now we're done with the evaluation. Since we don't need
           * to consider the right side of this operation again, we
           * skip this element by increasing the current index by two
           * instead of one.
           */
          index += 2;

          /**
           * Skip the remainder of the loop.
           */
          continue;

        } else {
          /**
           * If the operator is not '*' or '/', we add it to the
           * remaining_expression.
           */
          remaining_expression.push_back(input.children[index]);
        }
      } else {
        this->system.print_error_message(
            std::string("Unexpected operator found at an even index in the "
                        "expression."));
        return false;
      }

    } else {
      /**
       * If we don't have an operator, we simply add the part of the
       * expression to the remaining_expression for later evaluation.
       */
      remaining_expression.push_back(input.children[index]);
    }

    /**
     * Continue with the next part of the expression.
     */
    ++index;
  }

  /**
   * If we reach this point, all multiplications and divisions were
   * interpreted successfully.
   *
   * It remains to interpret the addition and subtraction operators if
   * there are any.
   */

  /**
   * If the remaining_expression consists of only one element, then
   * this is the result of the whole expression. We evaluate it and
   * return the result.
   */
  if (remaining_expression.size() == 1) {
    DLOG(INFO) << "Size of remaining expression is 1. Evaluating single result."
               << std::endl;
    /**
     * Interpret the single element of the expression.
     */
    bool interpretation_success =
        interpret_parse_result(remaining_expression[0], result);

    if (!interpretation_success) {
      this->system.print_error_message(
          std::string("Expression evaluated to a single term that could not be "
                      "interpreted."));
      return false;
    }

    return true;

  } else if (remaining_expression.size() > 1) {
    DLOG(INFO) << "Remaining expression has size "
               << remaining_expression.size() << std::endl;
    /**
     * If the remaining_expression contains more than one element, we
     * evaluate the whole expression which should consist of additions
     * and subtractions only.
     */

    /**
     * The variable that we use to create the final result.  Since
     * from now on we only have additions and subtractions, we can
     * safely initialize this with 0.
     */
    double final_result = 0;

    /**
     * This variable that holds the operator in each iteration.
     *
     * Initially the current operation will be '+' since the first
     * operand will be added to 0 (which is the current final_result).
     */
    std::string current_operation = "+";

    for (int index = 0; index < (int)remaining_expression.size(); ++index) {

      /**
       * Even positions are operands. We evaluate the operand and
       * perform the current_operation.
       */
      if (index % 2 == 0) {

        std::any operand_result;

        /**
         * Interpret the operand.
         */
        bool interpretation_success =
          interpret_parse_result(remaining_expression[index], operand_result);

        if (!interpretation_success) {
          this->system.print_error_message(
              std::string("Could not interpret operand '") +
              remaining_expression[index].value + "'.");
          return false;
        }

        /**
         * If the operand was interpreted successfully, we now try to
         * evaluate at it as a number.
         */
        double operand_value;

        try {
          operand_value = std::any_cast<double>(operand_result);
          DLOG(INFO) << "Operand " << index
                     << " in remaining expression was evaluated to '"
                     << operand_value << "'." << std::endl;
        } catch (const std::bad_any_cast &bac) {
          this->system.print_error_message(
              std::string("Interpretation failed: Operand '") +
              remaining_expression[index].value +
              "' could not be interpreted as number :" + bac.what());
          return false;
        }

        /**
         * Now depending on the current operator we either add or
         * subtract the current operand.
         */
        if ("+" == current_operation) {
          /**
           * Add the operand_value to the final_result.
           */
          final_result += operand_value;
          DLOG(INFO) << "Adding '" << operand_value
                     << "' to current result, yielding: '" << final_result
                     << "'." << std::endl;
        } else {
          /**
           * The only alternative is subtraction.
           */
          final_result -= operand_value;
          DLOG(INFO) << "Subtracting '" << operand_value
                     << "' from current result, yielding: '" << final_result
                     << "'." << std::endl;
        }
      } else {
        /**
         * If the index is odd, we expect an operator.
         */
        if (remaining_expression[index].type == Operator) {

          /**
           * Update the current operation.
           */
          current_operation = remaining_expression[index].value;
          DLOG(INFO) << "Operation updated for index " << index << ": '"
                     << current_operation << "'." << std::endl;
        } else {
          this->system.print_error_message(
              std::string("Expected operator but found '") +
              remaining_expression[index].value + "' instead.");
          return false;
        }
      }
    }

    /**
     * Everything went as expected.
     */
    result = final_result;
    return true;
  } else {
    /**
     * If the remaining_expression is empty, something went wrong.
     */
    this->system.print_error_message(
        std::string("Could not evaluate empty expression."));
    return false;
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
        std::string("Use of undeclared variable '") + input.value +
        "'. Declare the variable first using 'var " + input.value + " = ...");
  }

  return success;
}

bool Interpreter::interpret_function(const ParseResult &function_call,
                                     std::any &result) {

  DLOG(INFO) << "Trying to interpret function: '" << function_call.value
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();
  /**
   * We check whether we know this function and start the
   * corresponding interpretation.
   */
  if (function_call.type == Function) {

    /**
     * Now we check if we know about a builtin function with the
     * corresponding name.
     */
    std::unordered_map<std::string,
                       std::function<bool(Interpreter *, const ParseResult &,
                                          std::any &)>>::const_iterator
        position_of_function =
            this->builtin_functions.find(function_call.value);

    /**
     * If we did find the function, execute it.
     */
    if (position_of_function != this->builtin_functions.end()) {
      std::function<bool(Interpreter *, const ParseResult &, std::any &)>
          function = position_of_function->second;

      /**
       * Actually calling the function.
       */
      return function(this, function_call, result);
    } else {
      this->system.print_error_message(std::string("Could not interpret '") +
                                       function_call.value +
                                       "'. No function definition found.");
      return false;
    }
  } else {
    this->system.print_error_message(
        std::string("Could not interpret '") + function_call.value +
        "'. Expected function but found '" +
        System::name_for_type.at(function_call.type) + "' instead.");
    return false;
  }
}

// Functions:

bool Interpreter::interpret_arguments_from_function_call(
    const ParseResult &function_call,
    std::unordered_map<std::string, std::any> &arguments,
    const std::vector<std::string> &parameters_to_interpret) {

  DLOG(INFO) << "Interpreting arguments from function call '"
             << function_call.value << "'." << std::endl;

  /**
   * The input value should be the ParseResult containing the whole
   * function call.
   */
  if (function_call.type == Function) {
    /**
     * The function should have a single child, which is the argument
     * list.
     */
    if (function_call.children.size() == 1) {
      if (function_call.children[0].type == ArgumentList) {

        /**
         * Now we gather the values.
         */
        for (const ParseResult &argument : function_call.children[0].children) {

          if (argument.type == Argument) {
            std::string parameter_name = argument.value;

            /**
             * Check whether we should evaluate this argument now. If
             * the parameters_to_interpret vector is empty, we
             * interpret all arguments. Otherwise we check if the
             * current argument is contained.
             */
            if (parameters_to_interpret.empty() ||
                std::find(parameters_to_interpret.begin(),
                          parameters_to_interpret.end(),
                          parameter_name) != parameters_to_interpret.end()) {

              DLOG(INFO) << "Interpreting argument for parameter: '" << parameter_name
                         << "'." << std::endl;

              /**
               * The parameter has to have exactly one argument value
               * as its child.
               */
              if (argument.children.size() == 1) {
                std::any argument_value;
                bool success =
                  interpret_parse_result(argument.children[0], argument_value);

                /**
                 * If we couldn't interpret the argument, we return false. The
                 * resulting error will have been printed already.
                 */
                if (!success) {
                  return false;
                }

                /**
                 * If the argument value could be interpreted successfully, we
                 * store the resulting value using the argument_name.
                 */
                arguments[parameter_name] = argument_value;

              } else {
                this->system.print_error_message(
                    std::string("In function call '") + function_call.value +
                    "': Expected argument but found '" +
                    System::name_for_type.at(argument.type) + "' instead.");
                return false;
              }

            } else {
              DLOG(INFO)
                  << "Skipping interpretation of argument for parameter '"
                  << parameter_name << "'." << std::endl;
            }

          } else {
            this->system.print_error_message(
                std::string("In function call '") + function_call.value +
                "': Expected argument but found '" +
                System::name_for_type.at(argument.type) + "' instead.");
            return false;
          }
        }

        /**
         * When we reach the end of the loop this means that we have
         * successfully interpreted all arguments we wanted to
         * interpret.
         */
        return true;

      } else {
        this->system.print_error_message(
            std::string("In function call '") + function_call.value +
            "': Expected argument list but found '" +
            System::name_for_type.at(function_call.children[0].type) +
            "' instead.");
        return false;
      }
    } else {
      this->system.print_error_message(
          std::string("Could not interpret function '") + function_call.value +
          ": The function call contained more than the argument list.");
      return false;
    }
  } else {
    this->system.print_error_message(
        std::string("Unexpectedly found '") +
        System::name_for_type.at(function_call.type) +
        "' while interpreting function '" + function_call.value + "'.");
    return false;
  }
}

bool Interpreter::argument_value_for_parameter(
    const std::string &parameter,
    const std::unordered_map<std::string, std::any> &interpreted_arguments,
    std::any &result) {

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  std::unordered_map<std::string, std::any>::const_iterator position =
      interpreted_arguments.find(parameter);

  if (position != interpreted_arguments.end()) {
    result = position->second;
    return true;
  }

  /**
   * If we didn't find the value something went wrong.
   */
  return false;
}

bool Interpreter::number_value_for_parameter(
    const std::string &parameter,
    const std::unordered_map<std::string, std::any> &interpreted_arguments,
    double &value) {

  std::any argument_value;
  if (!(argument_value_for_parameter(parameter, interpreted_arguments,
                                     argument_value) &&
        number_from_value(argument_value, value))) {
    this->system.print_error_message(
        std::string("Could not interpret function . Argument for "
                    "parameter '") +
        parameter + "' could not be interpreted as number.");
    return false;
  }

  return true;
}

bool Interpreter::string_value_for_parameter(
    const std::string &parameter,
    const std::unordered_map<std::string, std::any> &interpreted_arguments,
    std::string &str) {

  /**
   * First we try to get the argument value from the argument list.
   */
  std::any argument_value;
  if (!(argument_value_for_parameter(parameter, interpreted_arguments,
                                     argument_value))) {
    this->system.print_error_message(
        std::string("Could not interpret function . Argument for "
                    "parameter '") +
        parameter + "' could not be interpreted as number.");
    return false;
  }

  /**
   * Now that we found the argument we try to cast it as string.
   */
  try {
    str = std::any_cast<std::string>(argument_value);
    return true;
  } catch (std::bad_any_cast &bac) {
    return false;
  }
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
