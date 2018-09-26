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
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include <glog/logging.h>

#include <canvas.hpp>
#include <lexer.hpp>

namespace hydra {
  class Interpreter {
  public:

    /**
     * Constructor
     */
    Interpreter(System &system);

    /**
     * The system knows about types and keywords, and additionally
     * knows holds the scopes, which the interpreter needs in order to
     * evaluate variables, etc.
     */
    System &system;

    /**
     * The canvas holds all geometric objects.
     */
    Canvas canvas;

    /**
     * Maps a Type (e.g. Assignment) to the function that is
     * responsible for interpreting ParseResults of this type.
     */
    std::unordered_map<
        Type,
        std::function<bool(Interpreter *, const ParseResult &, std::any &)>>
        known_interpretations;

    /**
     * Maps the function names to the corresponding implementation.
     */
    std::unordered_map<
        std::string,
        std::function<bool(Interpreter *, const ParseResult &, std::any &)>>
        builtin_functions;

    /**
     * Tries to cast a value to double. Returns false if the cast
     * failed.
     */
    bool number_from_value(const std::any &value, double &number);

    /**
     * Returns if a parse result contains an error.
     */
    bool parse_result_is_valid(const ParseResult &result);

    /**
     * Interprets a series of ParseResults.
     */
    bool interpret_code(const std::vector<ParseResult> &code, std::any &result);

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
     * Interprets a mathematical expression.  Returns false if an
     * error occurred during interpretation.  The result contains the
     * value of the interpretation.
     */
    bool interpret_expression(const ParseResult &input, std::any &result);

    /**
     * Interprets a function.  Returns false if an error occurred
     * during interpretation.  The result contains the value of the
     * interpretation.
     */
    bool interpret_function(const ParseResult &function_call, std::any &result);

    /**
     * Interprets an initialization.  Returns false if an error
     * occurred during interpretation.  The result contains the value
     * of the interpretation.
     */
    bool interpret_initialization(const ParseResult &initialization,
                                  std::any &result);

    /**
     * Interprets a loop.  Returns false if an error occurred during
     * interpretation.  The result contains the value of the
     * interpretation.
     */
    bool interpret_loop(const ParseResult &loop, std::any &result);

    /**
     * Interprets a number.  Returns false if an error occurred during
     * interpretation.  The result contains the value of the
     * interpretation.
     */
    bool interpret_number(const ParseResult &input, std::any &result);

    /**
     * Interprets a string.  Returns false if an error occurred during
     * interpretation.  The result contains the value of the
     * interpretation.
     */
    bool interpret_string(const ParseResult &input, std::any &result);

    /**
     * Interprets a ParseResult of unknown type.  Returns false if an
     * error occurred during interpretation.  The result contains the
     * value of the interpretation.
     */
    bool interpret_unknown(const ParseResult &input, std::any &result);

    /**
     * Tries to interpret a variable.  Returns false if an error
     * occurred during interpretation.  The result contains the value
     * of the interpretation.
     */
    bool interpret_variable(const ParseResult &input, std::any &result);

    // Functions:

    /**
     * Given a complete function call, tries to interpret the
     * arguments.
     *
     * Since some functions cannot interpret all arguments at once,
     * the parameters_to_interpret vector can by used to define which
     * arguments should be interpreted. By default the
     * parameters_to_interpret vector is empty, indicating that all
     * arguments should be interpreted.
     */
    bool interpret_arguments_from_function_call(
        const ParseResult &function_call,
        std::unordered_map<std::string, std::any> &arguments,
        const std::vector<std::string> &parameters_to_interpret = {});

    /**
     * Given an interpreted argument list, tries to determine the value
     * for the passed parameter name.  Returns false if the argument
     * value could not be obtained.  If successful, returns true and
     * passes the result to value.
     */
    bool argument_value_for_parameter(
        const std::string &parameter,
        const std::unordered_map<std::string, std::any> &interpreted_arguments,
        std::any &result);

    /**
     * Given an interpreted argument list, tries to determine the
     * numerical value for the passed parameter name.  Returns false,
     * if the value could not be obtained.  If successful, returns true and
     * passes the result to value.
     */
    bool number_value_for_parameter(
        const std::string &parameter,
        const std::unordered_map<std::string, std::any> &interpreted_arguments,
        double &value);

    /**
     * Given an interpreted argument list, tries to determine the
     * Pol value for the passed parameter name.  Returns false,
     * if the value could not be obtained.  If successful, returns true and
     * passes the result to value.
     */
    bool pol_value_for_parameter(
        const std::string &parameter,
        const std::unordered_map<std::string, std::any> &interpreted_arguments,
        Pol &value);

    /**
     * Given an interpreted argument list, tries to determine the
     * numerical value for the passed parameter name.  Returns false,
     * if the value could not be obtained.  If successful, returns true and
     * passes the result to value.
     */
    bool string_value_for_parameter(
        const std::string &parameter,
        const std::unordered_map<std::string, std::any> &interpreted_arguments,
        std::string &str);

    /**
     * The implementation of these functions can be found in
     * interpreter_functions.cpp
     */
    bool function_circle(const ParseResult &function_call, std::any &result);
    bool function_cos(const ParseResult &function_call, std::any &result);
    bool function_cosh(const ParseResult &function_call, std::any &result);
    bool function_exp(const ParseResult &function_call, std::any &result);
    bool function_log(const ParseResult &function_call, std::any &result);
    bool function_line(const ParseResult &function_call, std::any &result);
    bool function_print(const ParseResult &function_call, std::any &result);
    bool function_random(const ParseResult &function_call, std::any &result);
    bool function_save(const ParseResult &function_call, std::any &result);
    bool function_sin(const ParseResult &function_call, std::any &result);
    bool function_sinh(const ParseResult &function_call, std::any &result);

    /**
     * Determines the string representation of an interpretation
     * result. Returns false if no representation could be obtained.
     */
    static bool string_representation_of_interpretation_result(
        const std::any &result, std::string &str);

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
