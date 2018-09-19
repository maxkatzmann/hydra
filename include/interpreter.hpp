//
//  interpreter.hpp
//  hydra
//
//  Used to interpret hydra code.
//
//  Created by Maximilian Katzmann on 18.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef interpreter_hpp
#define interpreter_hpp

#include <any>
#include <string>
#include <vector>
#include <unordered_map>

#include <glog/logging.h>

#include <lexer.hpp>

namespace hydra {
  class Interpreter {
  public:

    Interpreter();

    Lexer lexer;

    /**
     * Holds the variables by name and value for all currently open
     * scopes.
     */
    std::vector<std::unordered_map<std::string, std::any>> scopes;

    /**
     * Determines the value for a variable and stores the result in
     * value.  If no value could be determined, value.has_value()
     * returns false.
     */
    void value_for_variable(const std::string &variable, std::any &value);

    /**
     * Determines the value for a variable in the current scope and
     * stores the result in value.  If no value could be determined,
     * value.has_value() returns false.
     */
    void value_for_variable_in_current_scope(const std::string &variable,
                                             std::any &value);

    /**
     * Returns if a parse result contains an error.
     */
    bool parse_result_is_valid(const ParseResult &result);

    /**
     * Interprets a parse result.  Returns false if an error occurred
     * during interpretation.  The result contains the value of the
     * interpretation.
     */
    bool interpret_parse_result(const ParseResult &input, std::any &result);

    /**
     * Interprets an assignment.  Returns false if an error occurred
     * during interpretation.  The result contains the value of the
     * interpretation.
     */
    bool interpret_assignment(const ParseResult &input, std::any &result);

    /**
     * Interprets an initialization.  Returns false if an error
     * occurred during interpretation.  The result contains the value
     * of the interpretation.
     */
    bool interpret_initialization(const ParseResult &input, std::any &result);

    /**
     * Interprets a number.  Returns false if an error occurred during
     * interpretation.  The result contains the value of the
     * interpretation.
     */
    bool interpret_number(const ParseResult &input, std::any &result);

    /**
     * Tries to find out what type the result is and cast it in order
     * to print it.
     */
    static bool print_interpretation_result(const std::any &result);

    /**
     * Print scopes.
     */
    void print_scopes();
    void print_scope_at_index(int index);
  };
}

#endif /* interpreter_hpp */
