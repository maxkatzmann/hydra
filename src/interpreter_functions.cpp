//
//  interpreter_functions.cpp
//  hydra
//
//  Contains the implementations of hydra's built-in functions.
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#include <interpreter.hpp>
#include <pol.hpp>

#include <cmath>
#include <iostream>
#include <random>

namespace hydra {

bool Interpreter::function_clear(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * If there are arguments, something went wrong. Clear is not
   * supposed to have arguments.
   */
  if (!interpreted_arguments.empty()) {
    this->system.print_error_message(
        std::string("Extraneous argument in call to function '") +
        function_call.value + "'. This function does not take any arguments.");
    return false;
  }

  /**
   * Actually clear the canvas.
   */
  this->canvas.clear();

  return true;
}

bool Interpreter::function_circle(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
   */
  Pol center;
  if (!pol_value_for_parameter("center", interpreted_arguments, center)) {
    return false;
  }

  double radius;

  if (!number_value_for_parameter("radius", interpreted_arguments, radius)) {
    return false;
  }

  /**
   * Add the circle to the canvas.
   */
  Path circle_path;
  Canvas::path_for_circle(center, radius, this->canvas.resolution, circle_path);
  this->canvas.add_path(circle_path);

  return true;
}

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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
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

bool Interpreter::function_curve_angle(const ParseResult &function_call,
                                       std::any &result) {
  DLOG(INFO) << "Interpreting " << function_call.value << "." << std::endl;
  /**
   * Reset the result so the check for has_value fails.
   */
  result.reset();

  /**
   * Interpret the arguments. Note, that we don't interpret all
   * arguments initially. That is because the last argument might
   * contain the hidden variable _p which will only be evaluated when
   * _p is known.
   */
  std::unordered_map<std::string, std::any> interpreted_arguments;
  if (!interpret_arguments_from_function_call(function_call, interpreted_arguments,
                                              {"from", "to"})) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
   */
  Pol from;
  if (!pol_value_for_parameter("from", interpreted_arguments, from)) {
    return false;
  }

  Pol to;
  if (!pol_value_for_parameter("to", interpreted_arguments, to)) {
    return false;
  }

  /**
   * Check whether they both have the same angular coordinate. If not
   * we print an error.
   */
  if (from.phi != to.phi) {
    this->system.print_error_message(
        std::string("Could not interpret '") + function_call.value +
        "'. The angular coordinates of the two endpoints did not match: '" +
        std::to_string(from.phi) + "' vs. '" + std::to_string(to.phi) + "'.");
    return false;
  }

  /**
   * Make sure that the from coordinate has a smaller radius than the
   * to coordinate.
   */
  if (from.r > to.r) {
    Pol temp = from;
    from = to;
    to = from;
  }

  /**
   * Add the curve to the canvas.
   */
  Path path;
  path.is_closed = false;

  /**
   * We create the points of the path by iteratively increasing the
   * radius, and evaluating the angle argument.
   */
  const double step_size = (to.r - from.r) / this->canvas.resolution;
  DLOG(INFO) << "Step size: " << step_size << std::endl;

  /**
   * If the step size is not positive, we will run into an infinite
   * loop. We rather throw an error before.
   */
  if (!(step_size > 0)) {
    this->system.print_error_message(
        std::string("Invalid step size <= 0 in function '") +
        function_call.value +
        "'. Make sure that 'to' and 'from' are not the same point.");
    return false;
  }

  double radius = from.r;

  /**
   * We add current point (on the line) as the hidden variable _p to
   * a new scope that will only live for this function execution.
   */
  this->system.state.open_new_scope();

  PropertyMap current_point;
  current_point[System::type_string] = "Pol";
  current_point["r"] = radius;
  current_point["phi"] = from.phi;

  const std::string hidden_variable_name = "_p";

  int current_scope =
      this->system.state.define_variable_with_value(hidden_variable_name, current_point);

  /**
   * In the loop we iteratively evaluate the angle argument.
   */
  std::unordered_map<std::string, std::any> interpreted_angle_argument;
  double angle = 0.0;

  while (radius <= to.r) {

    /**
     * Now that the hidden variable is defined, we interpret the angle
     * argument.
     */
    if (!interpret_arguments_from_function_call(function_call, interpreted_angle_argument,
                                                {"angle"})) {
      return false;
    }

    /**
     * We try to get the angle value.
     */
    if (!number_value_for_parameter("angle", interpreted_angle_argument,
                                    angle)) {
      return false;
    }

    /**
     * Create the point with the determined angle.
     */
    Pol point(radius, from.phi + angle);

    /**
     * Add the point to the path.
     */
    path.push_back(point);

    /**
     * Increase the radius.
     */
    radius += step_size;

    /**
     * Update the radius of the hidden variable _p.
     */
    current_point["r"] = radius;
    this->system.state.set_value_for_variable(hidden_variable_name,
                                              current_point, current_scope);
  }

  /**
   * We now close the scope that holds the hidden variable.
   */
  if (!this->system.state.close_scope()) {
    this->system.print_error_message(std::string(
        "Could not close hidden variable scope as that would mean closing the "
        "last scope."));
    return false;
  }

  /**
   * Actually adding the path to the canvas.
   */
  this->canvas.add_path(path);

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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
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

bool Interpreter::function_log(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * Compute the result.
   */
  result = ::log(x);
  return true;
}

bool Interpreter::function_line(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
   */
  Pol from;
  if (!pol_value_for_parameter("from", interpreted_arguments, from)) {
    return false;
  }

  Pol to;
  if (!pol_value_for_parameter("to", interpreted_arguments, to)) {
    return false;
  }

  /**
   * Add the line to the canvas.
   */
  Path line_path;
  Canvas::path_for_line(from, to, this->canvas.resolution, line_path);
  this->canvas.add_path(line_path);

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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument values.
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

bool Interpreter::function_save(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
   */
  std::string file_name;
  if (!string_value_for_parameter("file", interpreted_arguments, file_name)) {
    return false;
  }

  /**
   * Tell the canvas to save its contents to the passed file.
   */
  this->canvas.save_to_file(file_name);
  return true;
}

bool Interpreter::function_set_resolution(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * The step size has to be positive.
   */
  if (!(x > 0.0)) {
    this->system.print_error_message(
        std::string("Invalid argument in function '") + function_call.value +
        "'. Cannot set non-positive resolution.");
    return false;
  }

  /**
   * Set the resolution of the canvas.
   */
  this->canvas.resolution = x;
  result = x;
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
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

bool Interpreter::function_sqrt(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument value.
   */
  double x;

  if (!number_value_for_parameter("x", interpreted_arguments, x)) {
    return false;
  }

  /**
   * Compute the result.
   */
  result = ::sqrt(x);
  return true;
}

bool Interpreter::function_theta(const ParseResult &function_call,
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
  if (!interpret_arguments_from_function_call(function_call,
                                              interpreted_arguments)) {
    return false;
  }

  /**
   * Now we try to obtain the actual argument values.
   */
  double r_1;
  double r_2;
  double R;

  /**
   * Try interpreting the parameters.
   */
  if (!number_value_for_parameter("r1", interpreted_arguments, r_1) ||
      !number_value_for_parameter("r2", interpreted_arguments, r_2) ||
      !number_value_for_parameter("R", interpreted_arguments, R)) {
    return false;
  }

  /**
   * Check whether the argument value are valid.
   */
  if (r_1 > R || r_2 > R) {
    this->system.print_error_message(
        std::string("Could not interpret '") + function_call.value +
        "'. Argument 'r1' and 'r2' must not be larger than 'R'. (r1 = " +
        std::to_string(r_1) + ", r2 = " + std::to_string(r_2) +
        "', R = " + std::to_string(R) + ")");
    return false;
  }

  if (r_1 + r_2 < R) {
    this->system.print_error_message(
        std::string("Could not interpret '") + function_call.value +
        "'. The sum of the arguments 'r1' and 'r2' must be at least 'R'.");
    return false;
  }

  /**
   * Actually computing the value.
   */
  double theta = Pol::theta(r_1, r_2, R);

  /**
   * A value smaller 0.0 indicates that the value could not be
   * computed because of numerical issues.
   */
  if (theta < 0.0) {
    this->system.print_error_message(
        std::string("Could not interpret '") + function_call.value +
        "'. The value could not be computed due to numerical issues.");
    return false;
  }

  result = theta;
  return true;
}
}  // namespace hydra
