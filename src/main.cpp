/**
 * It is important that this macro is defined first such that later
 * includes acknowledge it.
 */
#ifndef DEBUG
#define NDEBUG
#endif

#include <any>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

/**
 * Hydra
 */
#include <lexer.hpp>
#include <interpreter.hpp>

/**
 * gflags / glog
 */
#include <gflags/gflags.h>
#include <glog/logging.h>

/**
 * Flags
 */
// DEFINE_string(flag, "", "A test flag that takes a string.");

/**
 * Main procedure
 */
int main(int argc, char *argv[]) {

  /**
   * Evaluating the flags.
   */
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  /**
   * A debug command that is only evaluated when the program is
   * compiled in debug mode.
   */
  DLOG(INFO) << "Hydra is starting." << std::endl;

  /**
   * Logging to console.
   */
  FLAGS_logtostderr = 1;

  hydra::Interpreter interpreter;

  hydra::Lexer lexer;
  // std::string command = "let o = Pol(r: 0.0, phi: 0.5 * M_PI)";
  std::string command = "let a = 5.0";

  hydra::ParseResult command_parse_result;
  lexer.parse_string(command, command_parse_result);
  hydra::Lexer::print_parse_result(command_parse_result);

  std::any interpretation_result;

  if (interpreter.interpret_parse_result(command_parse_result,
                                         interpretation_result)) {
    std::cout << "Successfully interpreted: '" << command_parse_result.value
              << "'." << std::endl;
    if (interpretation_result.has_value()) {
      std::cout << "Result has value: ";
      hydra::Interpreter::print_interpretation_result(interpretation_result);
      std::cout << std::endl;
    } else {
      std::cout << "Could not get interpretation result." << std::endl;
    }
  } else {
    std::cout << "Could not interpret '" << command_parse_result.value
              << "' successfully." << std::endl;
  }

  interpreter.print_scopes();

  return 0;
}
