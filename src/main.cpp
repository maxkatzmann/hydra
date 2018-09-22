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
#include <system.hpp>
#include <state.hpp>

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

  hydra::System system;
  hydra::Lexer lexer(system);
  hydra::Interpreter interpreter(system);

  // std::string command =
  //     "var o = Pol(r: 0.0, phi: random(from: 0.0 + 1.0, to: (0.5 * M_PI)))";

  std::vector<std::string> code = {
                                   "print(message: \"Bow to the mighty Hydra!\n\")"
  };

  std::cout << "Interpreting code: " << std::endl << std::endl;
  for (int line = 0; line < (int)code.size(); ++line) {
    std::cout << line + 1 << "| " << code[line] << std::endl;
  }
  std::cout << std::endl;

  for (int line_index = 0; line_index < (int)code.size(); ++line_index) {

    system.state.line_number = line_index + 1;
    system.state.current_line = code[line_index];

    std::cout << "Interpreting line " << system.state.line_number << ": '"
              << system.state.current_line << "'" << std::endl;

    std::vector<hydra::Token> tokens;
    hydra::ParseResult command_parse_result;

    bool success = lexer.parse_string(system.state.current_line, command_parse_result);
    if (!success) {
      std::cerr << "Could not parse command." << std::endl;
    }

    hydra::Lexer::print_parse_result(command_parse_result);

    std::any interpretation_result;

    if (interpreter.interpret_parse_result(command_parse_result,
                                           interpretation_result)) {
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
  }

  return 0;
}
