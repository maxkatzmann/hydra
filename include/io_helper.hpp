//
//  io_helper.hpp
//  hydra
//
//  Some methods that make some IO-Tasks more convenient.
//
//  Created by Maximilian Katzmann on 24.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef io_helper_hpp
#define io_helper_hpp

#include <functional>
#include <string>

namespace hydra {
class IOHelper {
 public:
  /**
   * Iterate the lines in the file with the passed file_name. For each
   * line there will be a line_read_callback that passes the read line
   * together with the line number. If the callback returns false, the
   * iteration will stop.
   */
  static void iterate_lines_in_file(
      const std::string &file_name,
      std::function<bool(const std::string &, const int)> line_read_callback);

  /**
   * Read the lines of a file into a vector of strings. Also actually
   * escapes newlines.
   */
  static void read_code_from_file(const std::string &file_name,
                                  std::vector<std::string> &code);
};
}  // namespace hydra

#endif /* io_helper_hpp */
