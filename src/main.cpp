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
#include <io_helper.hpp>
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
 * Forward declarations.
 */
void interpret_code_from_file(const std::string &file_name);

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


  /**
   * Check whether a file name was passed as argument.
   */
  if (argc > 1) {
    std::string file_name(argv[1]);

    /**
     * We got a file, so we try to interpret its code.
     */
    interpret_code_from_file(file_name);
  }

  return 0;
}

/**
 * Reads and interprets code from a hydra file.
 */
void interpret_code_from_file(const std::string &file_name) {

  hydra::System system;
  hydra::Lexer lexer(system);
  hydra::Interpreter interpreter(system);

  /**
   * Read the code from the passed file.
   */
  std::vector<std::string> code;

  DLOG(INFO) << "Reading code from file: '" << file_name << "'..." << std::endl;
  hydra::IOHelper::read_code_from_file(file_name, code);

  #ifdef DEBUG
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
  #endif

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
    std::cerr << "Code could not be interpreted successfully." << std::endl;
  }

  #ifdef DEBUG
  interpreter.print_scopes();
  #endif

}
