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
      // "for i in [0.0, 1.0, 3.0] {",
      // "\tfor j in [1.0, 1.0, i * i] {",
      // "\t\tprint(message: \"-\")",
      // "\t}",
      // "\tprint(message: \"> \")",
      // "\tprint(message: \"Bow to the mighty Hydra!\n\")",
      // "}",
      // "print(message: \"Done!\n\")"
                                   "var a = 5.0",
                                   "print(message: \"Five is \\(a) and Two times Five is \\(2.0 * a)\")",
                                   "print(message: \"Fiveteen is \\(1.0)\\(a)\")",
                                   "print(message: \"\\(M_PI) tastes great!\")"
  };

  std::cout << "Interpreting code: " << std::endl << std::endl;
  for (int line = 0; line < (int)code.size(); ++line) {
    std::string code_line = code[line];

    int position_of_new_line = code_line.find_first_of("\n");

    while (position_of_new_line != (int)std::string::npos) {
      code_line.replace(position_of_new_line, 1, "\\n");
      position_of_new_line = code_line.find_first_of("\n", position_of_new_line);
    }

    std::cout << line + 1 << "| " << code_line << std::endl;
  }
  std::cout << std::endl;

  /**
   * First parse the whole code.
   */
  std::vector<hydra::ParseResult> parsed_code;
  lexer.parse_code(code, parsed_code);

  /**
   * Print the parsed code.
   */
  #ifdef DEBUG
  for (const hydra::ParseResult &parsed_line : parsed_code) {
    std::cout << parsed_line.line_number << "| ";
    hydra::Lexer::print_parse_result(parsed_line);
  }
  #endif

  /**
   * Interpret the code.
   */
  std::any interpretation_result;
  if (!interpreter.interpret_code(parsed_code, interpretation_result)) {
    std::cerr << "Code could not be interpreted..." << std::endl;
  }

  #ifdef DEBUG
  interpreter.print_scopes();
  #endif

  return 0;
}
