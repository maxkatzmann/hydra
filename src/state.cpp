//
//  state.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 20.09.18.
//

#include <state.hpp>
#include <iostream>

namespace hydra {

State::State() {
  this->scopes.push_back(std::unordered_map<std::string, std::any>());
}

void State::open_new_scope() {
  this->scopes.push_back(std::unordered_map<std::string, std::any>());
}

bool State::close_scope() {

  /**
   * We must not delete the last scope.
   */
  if (this->scopes.size() <= 1) {
    return false;
  }

  this->scopes.pop_back();
  return true;
}

bool State::define_variable_with_value(const std::string &variable,
                                       const std::any &value) {
  /**
   * First check whether the variable is not yet defined.
   */
  std::any already_defined_value;
  if (value_for_variable_in_current_scope(variable, already_defined_value)) {
    /**
     * Variable is already defined.
     */
    return false;
  }

  /**
   * Check whether the value does actually have a value.
   */
  if (!value.has_value()) {
    return false;
  }

  /**
   * Actually defining the variable.
   */
  this->scopes.back()[variable] = value;
  return true;
}

bool State::set_value_for_variable(const std::string &variable,
                                   const std::any &value) {

  /**
   * We now check whether the variable is already defined.
   */
  std::any current_value;
  if (!value_for_variable(variable, current_value)) {
    return false;
  }

  /**
   * If the new value doesn't actually have a value, we cannot assign
   * it.
   */
  if (!value.has_value()) {
    return false;
  }

  /**
   * Actually assigning the value.
   */
  this->scopes.back()[variable] = value;
  return true;
}

bool State::value_for_variable(const std::string &variable, std::any &value) {
  /**
   * Reset the value to ensure that the has_value check fails when we
   * don't find a value.
   */
  value.reset();

  for (int i = this->scopes.size() - 1; i >= 0; --i) {
    std::unordered_map<std::string, std::any>::const_iterator
        position_of_variable = this->scopes[i].find(variable);

    if (position_of_variable != this->scopes[i].end()) {
      value = position_of_variable->second;
      return true;
    }
  }

  return false;
}

bool State::value_for_variable_in_current_scope(
    const std::string &variable, std::any &value) {

  /**
   * Reset the value to ensure that the has_value check fails when we
   * don't find a value.
   */
  value.reset();

  std::unordered_map<std::string, std::any>::const_iterator
      position_of_variable = this->scopes.back().find(variable);

  if (position_of_variable != this->scopes.back().end()) {
    value = position_of_variable->second;
    return true;
  }

  return false;
}

}  // namespace hydra
