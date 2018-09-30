//
//  io_helper.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 24.09.18.
//

#include <fstream>
#include <io_helper.hpp>

#include <glog/logging.h>

namespace hydra {
void IOHelper::iterate_lines_in_file(
    const std::string &file_name,
    std::function<bool(const std::string &, const int)> line_read_callback) {
  /**
   * Open the file.
   */
  std::ifstream file(file_name.c_str());

  /**
   * Check if the file could be opened.
   */
  CHECK(file.good()) << "Maybe the file " << file_name << " doesn't exist?"
                     << std::endl;

  /**
   * The variable that will hold each line.
   */
  std::string line;

  /**
   * We start with the first line.
   */
  int current_line_number = 1;

  /**
   * Iterate through the lines.
   */
  while (std::getline(file, line)) {
    /**
     * Do the callback. If the callback returns false, we stop
     * iterating the lines in the file.
     */
    if (!line_read_callback(line, current_line_number)) {
      break;
    }

    /**
     * Increase the line number.
     */
    ++current_line_number;
  }
}

void IOHelper::read_code_from_file(const std::string &file_name,
                                   std::vector<std::string> &code) {
  /**
   * Read lines from file into code vector.
   */
  hydra::IOHelper::iterate_lines_in_file(
      file_name,
      [&code](const std::string &line, const int line_number) -> bool {
        /**
         * We don't need the line_number.
         */
        (void)line_number;

        /**
         * We may need to manipulate the code_line.
         */
        std::string code_line = line;
        convert_new_lines(code_line);

        /**
         * Add the line of code.
         */
        code.push_back(code_line);

        /**
         * We don't stop iterating the file.
         */
        return true;
      });
}

void IOHelper::convert_new_lines(std::string &str) {
  /**
   * Replace \n with actual new line in the string. We somehow
   * can't search for '\\n' (that found find 'n' as
   * well). Therefore, we first search for '\' and check
   * whether the next character is n...
   */
  int position_of_possible_newline = str.find_first_of("\\");

  DLOG(INFO) << "Found possible newline at: " << position_of_possible_newline
             << std::endl;

  while (position_of_possible_newline != (int)std::string::npos &&
         position_of_possible_newline < (int)str.length() - 1) {
    DLOG(INFO) << "Position of possible new line is within bounds."
               << std::endl;

    if (str[position_of_possible_newline + 1] == 'n') {
      DLOG(INFO) << "Found n after possible new line." << std::endl;

      /**
       * Now we have found a new line.
       */
      str.replace(position_of_possible_newline, 2, "\n");
    }

    /**
     * Find the next new line.
     */
    position_of_possible_newline =
        str.find_first_of("\\", position_of_possible_newline + 1);
  }
}

}  // namespace hydra
