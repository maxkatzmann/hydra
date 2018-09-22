//
//  interpreter_functions.cpp
//  hydra
//
//  Contains the implementations of hydra's built-in functions.
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#include <interpreter.hpp>

#include <cmath>
#include <iostream>
#include <random>

namespace hydra {

bool Interpreter::function_cos(const ParseResult &function_call,
                                std::any &result) {
  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  interpret_arguments_from_function_call(function_call, interpreted_arguments);

  /**
   * Now we try to get the obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * Compute the result.
   */
  result = ::cos(x);
  return true;
}

bool Interpreter::function_cosh(const ParseResult &function_call,
                                std::any &result) {
  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  interpret_arguments_from_function_call(function_call, interpreted_arguments);

  /**
   * Now we try to get the obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * Compute the result.
   */
  result = ::cosh(x);
  return true;
}

bool Interpreter::function_exp(const ParseResult &function_call,
                               std::any &result) {
  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  interpret_arguments_from_function_call(function_call, interpreted_arguments);

  /**
   * Now we try to get the obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * Compute the result.
   */
  result = ::exp(x);
  return true;
}

bool Interpreter::function_print(const ParseResult &function_call,
                                 std::any &result) {
  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  interpret_arguments_from_function_call(function_call, interpreted_arguments);

  /**
   * Now we try to get the obtain the actual argument value.
   */
  std::string message;

  if (!string_value_for_parameter("message", interpreted_arguments, message)) {
    return false;
  }

  /**
   * Simply print the message;
   */
  std::cout << message;

  /**
   * Compute the result.
   */
  return true;
}

bool Interpreter::function_random(const ParseResult &function_call,
                                  std::any &result) {

  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  interpret_arguments_from_function_call(function_call, interpreted_arguments);

  /**
   * Now we try to get the obtain the actual argument values.
   */
  double from;
  double to;

  /**
   * Try interpreting the parameters.
   */
  if (!number_value_for_parameter("from", interpreted_arguments, from) ||
      !number_value_for_parameter("to", interpreted_arguments, to)) {
    return false;
  }

  /**
   * Check whether the lower bound is actually smaller than the upper
   * bound.
   */
  if (to < from) {
    this->system.print_error_message(
        std::string("Could not interpret '") + function_call.value +
        "'. Argument 'from' must not be larger than 'to'.");
    return false;
  }

  /**
   * Now that we have the lower bound (from) and the upper bound (to)
   * we draw a random double from this interval, which will then be
   * the result.
   */
  std::random_device random_device;
  std::mt19937 mersenne_twister(random_device());
  std::uniform_real_distribution<double> distribution(from, to);

  result = distribution(mersenne_twister);
  return true;
}

bool Interpreter::function_sin(const ParseResult &function_call,
                               std::any &result) {
  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  interpret_arguments_from_function_call(function_call, interpreted_arguments);

  /**
   * Now we try to get the obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * Compute the result.
   */
  result = ::sin(x);
  return true;
}

bool Interpreter::function_sinh(const ParseResult &function_call,
                                std::any &result) {
  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  interpret_arguments_from_function_call(function_call, interpreted_arguments);

  /**
   * Now we try to get the obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * Compute the result.
   */
  result = ::sinh(x);
  return true;
}
}
