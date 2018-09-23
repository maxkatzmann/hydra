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
      "for i in [0.0, 1.0, 3.0] {",
      "\tprint(message: \"Bow to the mighty Hydra!\")",
      "\tfor j in [0.0, 1.0, 2.0] {",
      "\t\tprint(message: \"-\")",
      "\t}",
      "}",
      "print(message: \"Done!\")"
                                   // "var a = M_PI * 2.0",
                                   // "var b = 3 * a",
                                   // "var c = random(from: a, to: b)"
  };

  std::cout << "Interpreting code: " << std::endl << std::endl;
  for (int line = 0; line < (int)code.size(); ++line) {
    std::cout << line + 1 << "| " << code[line] << std::endl;
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

  return 0;
}
