#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <deque>
#include <utility>
#include <functional>
#include <string>
#include "json_value.hpp"

namespace json
{

enum class e_Token : uint8_t
{
    LeftBrace = 0,
    RightBrace,
    LeftBracket,
    RightBracket,
    Colon,
    Comma,
    String,
    Integer,
    Float,
    True,
    False,
    Null,
    EndOfInput,
    Error
};

class Lexer
{
  public:
    struct TokenValue
    {
        const char* start;
        const char* end;
    };

    struct TokenError
    {
        const char* pos;
        const char* message;
    };
    
    struct Token
    {     
        e_Token type;
        union
        {
            TokenValue value;
            TokenError error;
        };

        Token() : type(e_Token::Error), value{ nullptr, nullptr } { }
        
        Token(e_Token t, const char* s, const char* e) : type(t), value{ s, e }
        {
        }

        Token(const char* s, const char* e) : type(e_Token::Error), error{ s, e }
        {
        }
    };

    Lexer() = delete;

    Lexer(const char* start, const char* end);

    Token next();
    Token peek();

  private:
    const char* m_pos;
    const char* m_end;

    Token read_string();
    Token read_number();
    Token read_potential_true();
    Token read_potential_false();
    Token read_potential_null();
};

struct Error
{
    const char* pos;
    const char* message;
    Error() : pos(nullptr), message(nullptr) { }
    Error(const char* p, const char* m) : pos(p), message(m) { }
};

class Parser
{
  public:
    Parser() = delete;

    Parser(const char* input);
    Parser(const char* start, const char* end);
    Parser(const std::string& input);

    void reset();
    void reset(const char* input);
    void reset(const char* start, const char* end);
    void reset(const std::string& input);

    Value parse(std::function<void(const Error&)> error_fun);

  private:
    const char* m_start;
    const char* m_end;
    Lexer m_lexer;

    enum class e_Expect : uint8_t
    {
        Value = 0,
        ValueOrClose,
        CommaOrClose,
        StringOrClose,
        String,
        Colon,
        EndOfInput
    };
    e_Expect m_expect;
    
    enum class e_Context : uint8_t
    {
        None,
        Object,
        Array
    };
    e_Context m_context;

    unsigned int m_length;

    struct stack_state
    {
        Value value;
        e_Context context;
        unsigned int length;

        stack_state(Value&& v, e_Context c, unsigned int l) 
            : value(std::forward<Value>(v)), context(c), length(l) { }
    };

    std::deque<stack_state> m_stack;

    unsigned int top_type() const;
    void check_expect(const Lexer::Token& token) const;
    void push(const Lexer::Token& token);
    void pop(const Lexer::Token& token);
    void pop_until_object(const Lexer::Token& token);
    void pop_until_array(const Lexer::Token& token);
    void comma_colon(const Lexer::Token& token);
    void error(const Lexer::Token& token);

    long long parse_integer(const Lexer::Token& token);
    double parse_float(const Lexer::Token& token);

    Value done(const Lexer::Token& token);
};

} //json

#endif //JSON_PARSER_H
