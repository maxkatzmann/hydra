//
//  interpreter.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#include <interpreter.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

namespace hydra {

Interpreter::Interpreter(System &system) : system(system) {

  this->builtin_functions = {
      {"clear", &Interpreter::function_clear},
      {"circle", &Interpreter::function_circle},
      {"cos", &Interpreter::function_cos},
      {"cosh", &Interpreter::function_cosh},
      {"exp", &Interpreter::function_exp},
      {"log", &Interpreter::function_log},
      {"line", &Interpreter::function_line},
      {"print", &Interpreter::function_print},
      {"save", &Interpreter::function_save},
      {"sin", &Interpreter::function_sin},
      {"sinh", &Interpreter::function_sinh},
      {"random", &Interpreter::function_random}
  };

  this->known_interpretations = {
      {Assignment, &Interpreter::interpret_assignment},
      {Initialization, &Interpreter::interpret_initialization},
      {Expression, &Interpreter::interpret_expression},
      {Function, &Interpreter::interpret_function},
      {Loop, &Interpreter::interpret_loop},
      {Number, &Interpreter::interpret_number},
      {String, &Interpreter::interpret_string},
      {Unknown, &Interpreter::interpret_unknown},
      {Variable, &Interpreter::interpret_variable}
  };

}

bool Interpreter::number_from_value(const std::any &value, double &number) {

  try {
    number = std::any_cast<double>(value);
    return true;
  } catch (std::bad_any_cast &bac) {
    return false;
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

bool Interpreter::interpret_code(const std::vector<ParseResult> &code,
                                 std::any &result) {
  /**
   * Interpret the ParseResults one after another.
   */
  for (const ParseResult &parsed_code : code) {

    /**
     * Try to interpret the parsed_code. If it fails, return false.
     */
    if (!interpret_parse_result(parsed_code, result)) {
      return false;
    }
  }

  /**
   * When we reach this point, everything went as expected.
   */
  return true;
}

bool Interpreter::interpret_parse_result(const ParseResult &input,
                                         std::any &result) {
  DLOG(INFO) << "Interpreting parse result of type: '" << System::name_for_type.at(input.type)
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = input.line_number;

  if (input.type == Error) {
    this->system.print_error_message(
        std::string("Cannot interpret statemet of type '") +
        System::name_for_type.at(input.type) + "'.");
    return false;
  }

  if (input.type == Empty) {
    return true;
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

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = input.line_number;

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
    if (input.children.size() != 3) {
      this->system.print_error_message(
          std::string("Invalid assignment. Use 'var a = 5.0' instead."));
      return false;
    }

    /**
     * The second parse result must be a variable name.
     */
    if (input.children[1].type != Variable) {
      this->system.print_error_message(
          std::string("Invalid assignment. Use 'var a = 5.0' instead."));
      return false;
    }

    /**
     * The variable name must not be empty.
     */
    if (input.children[1].value.empty()) {
      this->system.print_error_message(
          std::string("Invalid assignment: The variable name must not be "
                      "empty. Use 'var a = 5.0' instead."));
      return false;
    }

    /**
     * Variable assignments must not start with an
     * '_'. Underscores are reserved for internal variables.
     */
    if (input.children[1].value[0] == '_') {
      this->system.print_error_message(
          std::string("Invalid assignment. Variables starting with '_' "
                      "cannot be assigned to."));
      return false;
    }

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
      DLOG(INFO) << "Assignment value interpreted successfully." << std::endl;

      /**
       * Actually defining the variable.
       */
      if (!this->system.state.define_variable_with_value(
              input.children[1].value, value_interpretation_result)) {
        /**
         * Defining the variable can fail for two reasons. Either the
         * value didn't actually have a value, or the variable was
         * defined already..
         */
        if (!value_interpretation_result.has_value()) {
          this->system.print_error_message(
              std::string("Could not define '") + input.children[1].value +
              "'. Right hand side of assignment did not have a value.");
        } else {
          /**
           * If we did have a value but definition failed, then it was
           * because the variable already existed.
           */
          this->system.print_error_message(std::string("Redefinition of : '") +
                                           input.children[1].value + "'.");
        }

        return false;
      }

      /**
       * Everything worked as expected.
       */
      result = value_interpretation_result;
      return true;
    }
  } else {
    /**
     * Version 2 (without 'var'):
     */

    /**
     * The assignment should consist of exactly two values: the name
     * of the variable and whatever is being assigned to it.
     */
    if (input.children.size() != 2) {
      this->system.print_error_message(
          std::string("Invalid assignment. Use 'a = 5.0' instead."));
      return false;
    }

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
      DLOG(INFO) << "Assignment value interpreted successfully." << std::endl;

      /**
       * Check whether the variable could be assigned successfully.
       */
      if (!this->system.state.set_value_for_variable(
              input.children[0].value, value_interpretation_result)) {
        /**
         * If the assignment fails, there are two reasons. Either the
         * value was empty, or the variable was undefined.
         */
        /**
         * Check whether we actually got a value from the
         * interpretation.
         */
        if (!value_interpretation_result.has_value()) {
          this->system.print_error_message(
              std::string("Could not define '") + input.children[0].value +
              "'. Right hand side of assignment did not have a value.");
        } else {
          /**
           * If the result had a value, then the assignment failed
           * because the variable was not defined yet.
           */
          this->system.print_error_message(
              std::string(
                  "Trying to assign to undefined variable. Define the variable "
                  "first using 'var ") +
              input.children[0].value + " = ...' instead.");
        }

        return false;
      }

      result = value_interpretation_result;
      return true;
    }
  }

  return false;
}

bool Interpreter::interpret_initialization(const ParseResult &initialization,
                                           std::any &result) {
  DLOG(INFO) << "Interpreting initialization with value: '" << initialization.value
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = initialization.line_number;

  /**
   * Check whether the input is actually an initialization.
   */
  if (initialization.type != Initialization) {
    this->system.print_error_message(
        std::string("Unexpectedly found '") +
        System::name_for_type.at(initialization.type) +
        "' while interpreting initialization.");
    return false;
  }

  /**
   * First we try to get the arguments for the initialization. This
   * works the same way as with function arguments.
   *
   * The initialization should have exactly one child: the argument
   * list.
   */
  if (initialization.children.size() != 1) {
    this->system.print_error_message(
        std::string("Invalid number of arguments in initialization. Expected "
                    "one but found ") +
        std::to_string(initialization.children.size()) + " instead.");
    return false;
  }

  /**
   * Check whether the single child is an ArgumentList.
   */
  if (initialization.children[0].type != ArgumentList) {
    this->system.print_error_message(
        std::string("Unexpectedly found ") +
        System::name_for_type.at(initialization.children[0].type) +
        " while interpreting argument list.");
    return false;
  }

  /**
   * Now we actually interpret the argument list.
   */
  std::unordered_map<std::string, std::any> arguments;
  interpret_arguments_from_function_call(initialization, arguments);

  /**
   * Now depending on which type to evaluate we initialize it.
   */

  /**
   * Evaluating Pol (Polar coordinates).
   */
  if (initialization.value == "Pol") {

    /**
     * Get the actual argument values.
     */
    double r;
    double phi;

    /**
     * Get the numbers for the parameters.
     */
    if (!number_value_for_parameter("r", arguments, r) ||
        !number_value_for_parameter("phi", arguments, phi)) {
      return false;
    }

    /**
     * If we're at this point, we all we need for the initialization.
     */
    result = Pol(r, phi);
    return true;
  } else {
    this->system.print_error_message(std::string("Could not interpret '") +
                                     initialization.value +
                                     "'. No initialization definition found.");
    return false;
  }

  return false;
}

bool Interpreter::interpret_loop(const ParseResult &loop, std::any &result) {

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = loop.line_number;

  /**
   * Check whether we're actually dealing with a loop.
   */
  if (loop.type != Loop) {
    this->system.print_error_message(std::string("Unexpectedly found '") +
                                     System::name_for_type.at(loop.type) +
                                     "' instead.");
    return false;
  }

  /**
   * The children of a loop parse result are the following:
   *
   * the loop variable
   * the range
   * the statements in the loop.
   *
   * This means, if we don't have at least 3 children, something is
   * wrong.
   */
  if (loop.children.size() < 3) {
    this->system.print_error_message(
        std::string("Invalid number of arguments for loop."));
    return false;
  }

  /**
   * We open a new scope, since all variables defined in the loop (as
   * well as the loop variable) will be forgotten after the loop.
   */
  this->system.state.open_new_scope();

  /**
   * The first child of the loop-parse-result is the variable
   * name.
   */
  if (loop.children[0].type != Unknown && loop.children[0].type != Variable) {
    this->system.print_error_message(
        std::string(
            "Invalid syntax in loop definition. Expected variable name but found '") +
        System::name_for_type.at(loop.children[0].type) + "' instead.");
    return false;
  }

  /**
   * Get the loop variable name.
   */
  std::string loop_variable_name = loop.children[0].value;

  /**
   * The next child should be the range.
   */
  if (loop.children[1].type != Range) {
    this->system.print_error_message(
        std::string(
            "Invalid syntax in loop definition. Expected Range but found '") +
        System::name_for_type.at(loop.children[1].type) + "' instead.");
    return false;
  }

  /**
   * The range should have exactly 3 children.
   *
   * lowerbound stepsize upperbound
   */
  if (loop.children[1].children.size() != 3) {
    this->system.print_error_message(
        std::string("Invalid number of argument in range definition. Expected "
                    "3 arguments but found ") +
        std::to_string(loop.children[1].children.size()) + "' instead.");
    return false;
  }

  /**
   * Interpret the lower bound of the range.
   */
  std::any lower_bound_result;
  double lower_bound;
  if (!(interpret_parse_result(loop.children[1].children[0],
                               lower_bound_result) &&
        number_from_value(lower_bound_result, lower_bound))) {
    this->system.print_error_message(std::string(
        "Interpretation failed. Could not interpret lower bound of range."));
    return false;
  }

  /**
   * Interpret the step size of the range.
   */
  std::any step_size_result;
  double step_size;
  if (!(interpret_parse_result(loop.children[1].children[1],
                               step_size_result) &&
        number_from_value(step_size_result, step_size))) {
    this->system.print_error_message(std::string(
        "Interpretation failed. Could not interpret step size of range."));
    return false;
  }

  /**
   * Interpret the upper bound of the range.
   */
  std::any upper_bound_result;
  double upper_bound;
  if (!(interpret_parse_result(loop.children[1].children[2],
                               upper_bound_result) &&
        number_from_value(upper_bound_result, upper_bound))) {
    this->system.print_error_message(std::string(
        "Interpretation failed. Could not interpret upper bound of range."));
    return false;
  }

  /**
   * At this point we interpreted the whole range. Now we actually
   * loop.
   *
   * We start by assigning the lower_bound to the loop variable in the
   * current scope.
   */
  double loop_variable = lower_bound;
  std::any loop_variable_value = loop_variable;
  this->system.state.define_variable_with_value(loop_variable_name,
                                                loop_variable_value);

  /**
   * Now loop! We loop as long as the loop variable is smaller than
   * the upper bound.
   */

  DLOG(INFO) << "Loop variable is " << loop_variable << ", upper bound is "
             << upper_bound << std::endl;

  while (loop_variable <= upper_bound) {
    /**
     * Interpret the code within the loop. Since the first to children
     * of the loop are variable name and range, this leaves all later
     * children as contents of the loop.
     */
    DLOG(INFO) << "Interpreting loop with variable " << loop_variable_name
               << " = " << loop_variable << std::endl;
    for (int index = 2; index < (int)loop.children.size(); ++index) {
      /**
       * If interpreting this ParseResult fails, the whole loop fails.
       */
      std::any interpretation_result;
      if (!interpret_parse_result(loop.children[index], interpretation_result)) {
        return false;
      }
    }

    /**
     * The code in the loop was interpreted successfully. Now we
     * determine the value of the loop variable, which might have
     * changed during the loop and then update it by increasing it by
     * the step size.
     */

    /**
     * First check whether we can get the value for the loop
     * variable. We only allow to take the variable from the current
     * scope.
     */
    if (!this->system.state.value_for_variable_in_current_scope(
            loop_variable_name, loop_variable_value) &&
        number_from_value(loop_variable_value, loop_variable)) {
      this->system.print_error_message(
          std::string("Could not interpret loop. Loop variable '") +
          loop_variable_name + "' could not be interpreted as number.");
      return false;
    }

    /**
     * The loop_variable now has the current value. Update it by
     * increasing it by the step_size.
     */
    loop_variable += step_size;

    /**
     * Now we set the new value in the current scope.
     */
    loop_variable_value = loop_variable;
    if (!this->system.state.set_value_for_variable(loop_variable_name,
                                                   loop_variable_value)) {
      this->system.print_error_message(
          std::string(
              "Could not interpret loop. Unable to update loop variable '") +
          loop_variable_name + "'.");
      return false;
    }
  }

  /**
   * We're done interpreting the loop. We now remove the scope of the
   * loop including all the variables defined in there.
   */
  if (!this->system.state.close_scope()) {
    this->system.print_error_message(
        std::string("Could not close loop-scope as that would mean closing the "
                    "last scope."));
    return false;
  }

  /**
   * If we get here, everything went as expected.
   */
  return true;
}

bool Interpreter::interpret_number(const ParseResult &input, std::any &result) {

  DLOG(INFO) << "Interpreting number with value: '" << input.value
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = input.line_number;

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
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = input.line_number;

  /**
   * Reset the result so the check for has_value fails.
   */
  if (input.type != String) {
    this->system.print_error_message(
        std::string("Interpretation failed. Unexpectedly found '") +
        System::name_for_type.at(input.type) +
        "' while interpreting a string.");
    return false;
  }

  /**
   * Now we check if the string contains escape sequences. If the
   * string has children, its children form the actual string.
   */

  if (input.children.empty()) {
    result = input.value;
    return true;
  }

  /**
   * If we end up here, the string has children, which means that the
   * children form the actual string.
   */
  std::string final_string;

  /**
   * We iterate the parts of the string and try to evaluate each as
   * string.
   */
  for (const ParseResult &string_part : input.children) {

    /**
     * Try to interpret this string part.
     */
    std::any string_part_value;
    if (!interpret_parse_result(string_part, string_part_value)) {
      return false;
    }

    /**
     * We have the value. We now check whether we can get a string
     * representation.
     */
    std::string string_representation;
    if (!Interpreter::string_representation_of_interpretation_result(
            string_part_value, string_representation)) {
      this->system.print_error_message(std::string("Interpretation failed. '") +
                                       string_part.value +
                                       "' could not be interpreted as string.");
      return false;
    }

    /**
     * Add the interpreted part to the final_string.
     */
    final_string += string_representation;
  }

  /**
   * If we reached this point, everything went as expected.
   */
  result = final_string;
  return true;
}

bool Interpreter::interpret_unknown(const ParseResult &input,
                                    std::any &result) {

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = input.line_number;

  /**
   * If the input type is unknown we check whether it might be a
   * variable.  (The lexer doesn't know about the defined variables.)
   */
  if (input.type != Unknown) {
    /**
     * If the input type is not unknown something is wrong.
     */
    this->system.print_error_message(
        std::string("Interpretation failed. Unexpectedly found '") +
        System::name_for_type.at(input.type) +
        "' while interpreting something unknown.");
    return false;
  }

  /**
   * If the type is unknown we assume that its a variable. This
   * only works, if the input has no children.
   */
  if (!input.children.empty()) {
    return false;
  }

  return interpret_variable(input, result);
}

bool Interpreter::interpret_expression(const ParseResult &input,
                                       std::any &result) {

  DLOG(INFO) << "Interpreting mathematical expression." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = input.line_number;

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
      if (index % 2 != 1) {
        this->system.print_error_message(
            std::string("Unexpected operator found at an even index in the "
                        "expression."));
        return false;
      }

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
        if (remaining_expression[index].type != Operator) {
          this->system.print_error_message(
              std::string("Expected operator but found '") +
              remaining_expression[index].value + "' instead.");
          return false;
        }

        /**
         * Update the current operation.
         */
        current_operation = remaining_expression[index].value;
        DLOG(INFO) << "Operation updated for index " << index << ": '"
                   << current_operation << "'." << std::endl;
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
  DLOG(INFO) << "Interpreting variable with value: '" << input.value
             << "'." << std::endl;

  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = input.line_number;

  /**
   * We now check whether the variable is defined.
   */
  bool success = this->system.state.value_for_variable(input.value, result);

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
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = function_call.line_number;

  /**
   * We check whether we know this function and start the
   * corresponding interpretation.
   */
  if (function_call.type != Function) {
    this->system.print_error_message(
        std::string("Could not interpret '") + function_call.value +
        "'. Expected function but found '" +
        System::name_for_type.at(function_call.type) + "' instead.");
    return false;
  }

  /**
   * Now we check if we know about a builtin function with the
   * corresponding name.
   */
  std::unordered_map<std::string,
                     std::function<bool(Interpreter *, const ParseResult &,
                                        std::any &)>>::const_iterator
      position_of_function = this->builtin_functions.find(function_call.value);

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
}

// Functions:

bool Interpreter::interpret_arguments_from_function_call(
    const ParseResult &function_call,
    std::unordered_map<std::string, std::any> &arguments,
    const std::vector<std::string> &parameters_to_interpret) {

  DLOG(INFO) << "Interpreting arguments from function call '"
             << function_call.value << "'." << std::endl;

  /**
   * Let the system state now, which line we're currently
   * interpreting.
   */
  this->system.state.line_number = function_call.line_number;

  /**
   * The input value should be the ParseResult containing the whole
   * function call / initialization.
   */
  if (function_call.type != Function && function_call.type != Initialization) {
    this->system.print_error_message(
        std::string("Unexpectedly found '") +
        System::name_for_type.at(function_call.type) +
        "' while interpreting function '" + function_call.value + "'.");
    return false;
  }

  /**
   * The function should have a single child, which is the argument
   * list.
   */
  if (function_call.children.size() != 1) {
    this->system.print_error_message(
        std::string("Could not interpret function '") + function_call.value +
        ": The function call contained more than the argument list.");
    return false;
  }

  if (function_call.children[0].type != ArgumentList) {
    this->system.print_error_message(
        std::string("In function call '") + function_call.value +
        "': Expected argument list but found '" +
        System::name_for_type.at(function_call.children[0].type) +
        "' instead.");
    return false;
  }

  /**
   * Now we gather the values.
   */
  for (const ParseResult &argument : function_call.children[0].children) {
    /**
     * Check whether we're dealing with an argument.
     */
    if (argument.type != Argument) {
      this->system.print_error_message(
          std::string("In function call '") + function_call.value +
          "': Expected argument but found '" +
          System::name_for_type.at(argument.type) + "' instead.");
      return false;
    }

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
      if (argument.children.size() != 1) {
        this->system.print_error_message(
            std::string("In function call '") + function_call.value +
            "': Expected argument but found '" +
            System::name_for_type.at(argument.type) + "' instead.");
        return false;
      }

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
      DLOG(INFO) << "Skipping interpretation of argument for parameter '"
                 << parameter_name << "'." << std::endl;
    }
  }

  /**
   * When we reach the end of the loop this means that we have
   * successfully interpreted all arguments we wanted to
   * interpret.
   */
  return true;
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
        std::string(
            "Could not interpret function / initialization. Argument for "
            "parameter '") +
        parameter + "' could not be interpreted as number.");
    return false;
  }

  return true;
}

bool Interpreter::pol_value_for_parameter(
    const std::string &parameter,
    const std::unordered_map<std::string, std::any> &interpreted_arguments,
    Pol &value) {

  /**
   * Check whether the argument was is in the list.
   */
  std::any argument_value;
  if (!argument_value_for_parameter(parameter, interpreted_arguments,
                                    argument_value)) {
    this->system.print_error_message(
        std::string(
            "Could not interpret function / initialization. Argument for "
            "parameter '") +
        parameter + "' could not be found.");
    return false;
  }

  /**
   * Check whether the value can be cast to Pol.
   */
  try {
    value = std::any_cast<Pol>(argument_value);
    return true;
  } catch (std::bad_any_cast &bac) {
    this->system.print_error_message(
        std::string(
            "Could not interpret function / initialization. Argument for "
            "parameter '") +
        parameter + "' could not be interpreted as Pol.");
    return false;
  }

  return false;
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

bool Interpreter::string_representation_of_interpretation_result(
    const std::any &result, std::string &str) {

  /**
   * Check whether it is a double.
   */
  try {
    str = std::to_string(std::any_cast<double>(result));
    return true;
  } catch (const std::bad_any_cast &e) {
  }

  /**
   * Check whether it is a string.
   */
  try {
    str = std::any_cast<std::string>(result);
    return true;
  } catch (const std::bad_any_cast &e) {
  }

  /**
   * Check whether is a Pol object.
   */
  try {
    str = std::any_cast<Pol>(result).to_string();
    return true;
  } catch (const std::bad_any_cast &bac) {
  }

  /**
   * If we didn't find a string representation, we return false.
   */
  return false;
}

bool Interpreter::print_interpretation_result(const std::any &result) {

  std::string string_representation;
  if (Interpreter::string_representation_of_interpretation_result(
          result, string_representation)) {
    std::cout << string_representation;
    return true;
  }

  /**
   * If we didn't get a string representation, we couldn't print the
   * result.
   */
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
    Interpreter::print_interpretation_result(
        this->system.state.scopes[index].at(keys[i]));
    std::cout << "'" << std::endl;
  }
}

}  // namespace hydra
