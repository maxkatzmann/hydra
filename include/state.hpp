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

#include <canvas.hpp>

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

  /**
   * Opens a new scope.
   */
  void open_new_scope();

  /**
   * Closes / removes the current scope. Note that the very first
   * scope cannot be closed.
   *
   * Returns false, if the last remaining scope would have been
   * deleted that way.
   */
  bool close_scope();

  /**
   * Define the variable with the passed value. Variables are always
   * defined in the current scope. Returns the index of the scope that
   * the variable was defined in or -1 if the assignment failed..
   */
  int define_variable_with_value(const std::string &variable,
                                 const std::any &value);

  /**
   * Sets the value of the current variable in the passed
   * scope. Returns false if the variable did not exist in this scope.
   */
  bool set_value_for_variable(const std::string &variable,
                              const std::any &value, int scope);

  /**
   * Determines the value for a variable and stores the result in
   * value.  Returns the index of the scope, that the variable is
   * defined in. If no value could be determined, value_for_variable
   * returns -1 and value.has_value() returns false.
   */
  int value_for_variable(const std::string &variable, std::any &value);

  /**
   * Determines the value for a variable in the current scope and
   * stores the result in value.  If no value could be determined,
   * value.has_value() returns false.
   */
  bool value_for_variable_in_current_scope(const std::string &variable,
                                           std::any &value);

};
}  // namespace hydra

#endif /* state_hpp */
