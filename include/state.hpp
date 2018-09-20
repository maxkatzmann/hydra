//
//  system.hpp
//  hydra
//
//  Represents the state of a hydra program.
//
//  Created by Maximilian Katzmann on 20.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef state_hpp
#define state_hpp

#include <any>
#include <string>
#include <unordered_map>
#include <vector>

namespace hydra {
/**
 * Used to store a state of a hydra program. E.g., the current line
 * number, the current line, the scopes containing the variables.
 */
class State {

 public:

  State();

  int line_number = -1;

  std::string current_line = "";

  /**
   * Holds the variables by name and value for all currently open
   * scopes.
   */
  std::vector<std::unordered_map<std::string, std::any>> scopes;

};
}  // namespace hydra

#endif /* state_hpp */
