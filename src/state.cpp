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

}  // namespace hydra
