//
//  system.hpp
//  hydra
//
//  Represents the hydra system.
//
//  Created by Maximilian Katzmann on 19.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef system_hpp
#define system_hpp

#include <string>
#include <unordered_map>
#include <vector>

namespace hydra {

enum Type {
  Argument = 0,
  ArgumentList = 1,
  Assignment = 2,
  Empty = 3,
  Error = 4,
  Expression = 5,
  Function = 6,
  Loop = 7,
  Initialization = 8,
  Number = 9,
  Operator = 10,
  Range = 11,
  Unknown = 12,
  Variable = 13
};

class System {

public:
 static const std::string error_string;

 /**
  * Assigns each type the corresponding name.
  */
 static const std::unordered_map<Type, std::string, std::hash<int>>
     name_for_type;

 /**
  * Contains all known keywords and their associated types.  E.g.:
  * "let" is of type Assignment.
  */
 static const std::unordered_map<std::string, Type> types_for_keywords;

 /**
  * Contains all known functions and the associated arguments. E.g.:
  * "line -> from:to:"
  */
 static const std::unordered_map<std::string, std::vector<std::string>>
     arguments_for_functions;

 /**
  * Prints a vector of strings a argument list.
  */
 static void print_argument_list(const std::vector<std::string> &arguments);
};

} // namespace hydra

#endif /* system_hpp */
