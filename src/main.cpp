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
void launch_REPL();
void convert_new_lines(std::string &str);

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
  } else {

    /**
     * If we didn't get an input file, we start the REPL.
     */
    launch_REPL();
  }

  return 0;
}

/**
 * Reads and interprets code from a hydra file.
 */
void interpret_code_from_file(const std::string &file_name) {

  /**
   * What we need to interpret code is a System, the lexer that parses
   * the code and an interpreter.
   */
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

    /**
     * Replace '\n' with actual 'new lines'.
     */
    convert_new_lines(code_line);

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

/**
 * Launches the REPL!
 */
void launch_REPL() {

  std::cout << "Launching Hydra REPL:" << std::endl;

  /**
   * What we need to interpret code is a System, the lexer that parses
   * the code and an interpreter.
   */
  hydra::System system;
  hydra::Lexer lexer(system);
  hydra::Interpreter interpreter(system);

  /**
   * We usually interpret the code straight after execution. If,
   * however, we have a for loop, we only interpret the code once the
   * last for-loop closes.
   */
  std::vector<std::string> code;
  std::vector<hydra::ParseResult> parsed_code;

  /**
   * Get input from the user as long as the user didn't quit.
   */
  bool user_quit = false;

  /**
   * The string that represents the code input by the user.
   */
  std::string code_line = "";

  /**
   * We keep track of the number of opened loops. Usually we execute
   * code, as soon as its entered. However, when a loop is open, we
   * wait for the loop to be closed before actually interpreting.
   */
  int number_of_open_for_loops = 0;

  while (!user_quit) {

    std::cout << "[hydra] " << code.size() + 1 << "> ";

    for (int loop_number = 0; loop_number < number_of_open_for_loops; ++loop_number) {
      std::cout << "\t";
    }

    /**
     * Get the code from the console input.
     */
    std::getline(std::cin, code_line);

    DLOG(INFO) << "User entered code line: '" << code_line << "'" << std::endl;

    /**
     * Convert '\n' to actual new lines.
     */
    hydra::IOHelper::convert_new_lines(code_line);

    /**
     * When the user enters 'quit', we stop.
     */
    if (code_line == "quit") {
      user_quit = true;
      break;
    }

    /**
     * Save the code line for later interpretation. Since parsing
     * loops is not trivially done by iteratively parsing the lines
     * of code, we reparse the whole loop code when the loop is
     * done.
     */
    code.push_back(code_line);

    /**
     * For better error messages.
     */
    system.state.line_number = code.size();

    /**
     * If the user didn't quit, we try to parse the entered code.
     */
    hydra::ParseResult parse_result;
    if (!lexer.parse_string(code_line, parse_result)) {
      std::cerr << "Could not parse code. Not interpreting." << std::endl;

      /**
       * An error occurred we start over, with out having any code
       * stored.
       */
      number_of_open_for_loops = 0;
      code.clear();
      parsed_code.clear();
    }

    /**
     * If we're dealing with a loop, we do not interpret the code
     * immediately.
     */
    if (parse_result.type == hydra::Loop) {
      ++number_of_open_for_loops;
    } else if (parse_result.type == hydra::Braces) {
      /**
       * If we find a parenthesis, it is closing a loop.
       */
      --number_of_open_for_loops;
    }

    /**
     * If we don't have any open loops, we interpret the code.
     */
    if (number_of_open_for_loops <= 0) {

      if (!lexer.parse_code(code, parsed_code)) {
        std::cerr << "Could not parse code. Not interpreting." << std::endl;

        /**
         * An error occurred we start over, with out having any code
         * stored.
         */
        number_of_open_for_loops = 0;
        code.clear();
        parsed_code.clear();
      }

      std::any result;
      if (!interpreter.interpret_code(parsed_code, result)) {
        std::cerr << "(Code was not interpreted.)" << std::endl;

        /**
         * An error occurred we start over, with out having any code
         * stored.
         */
        number_of_open_for_loops = 0;
        code.clear();
        parsed_code.clear();
      }

      /**
       * If we did interpret the code, we try to get a string
       * representation of the interpretation result and print it.
       */
      std::string result_string;
      if (interpreter.string_representation_of_interpretation_result(result, result_string)) {
        std::cout << "> " << result_string << std::endl;
      }

      /**
       * Since we don't want to parse this same code again, we clear
       * the currently read code.
       */
      code.clear();
      parsed_code.clear();
    }
  }

  std::cout << "Exiting Hydra REPL." << std::endl;
}

/**
 * Takes a string and turns 'new lines' into '\n'.
 */
void convert_new_lines(std::string &str) {
  int position_of_new_line = str.find_first_of("\n");

  while (position_of_new_line != (int)std::string::npos) {
    str.replace(position_of_new_line, 1, "\\n");
    position_of_new_line = str.find_first_of("\n", position_of_new_line);
  }
}
