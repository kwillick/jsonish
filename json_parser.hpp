#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <deque>
#include <utility>
#include <functional>
#include "json_value.hpp"

namespace json
{

enum class e_Token
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
        
        Token(e_Token t, const char* s, const char* e) : type(t), value{s, e}
        {
        }

        Token(const char* s, const char* e) : type(e_Token::Error), error{s, e}
        {
        }
    };

    Lexer() = delete;

    Lexer(const char* start, const char* end);

    Token next();
  private:
    const char* m_pos;
    const char* m_end;

    Token read_string();
    Token read_number();
    Token read_potential_true();
    Token read_potential_false();
    Token read_potential_null();
};

class Parser
{
  public:
    struct Error
    {
        const char* pos;
        const char* message;
        Error(const char* p, const char* m) : pos(p), message(m) { }
    };

    Parser() = delete;

    Parser(const char* input);
    Parser(const char* start, const char* end);

    void reset();
    void reset(const char* input);
    void reset(const char* start, const char* end);

    Value parse(std::function<void(const Error&)> error_fun);

  private:
    const char* m_start;
    const char* m_end;
    Lexer m_lexer;

    std::deque<Value> m_stack;

    unsigned int top_type() const;
    void push(const Lexer::Token& token);
    void pop(const Lexer::Token& token);
    void error(const Lexer::Token& token);

    long long parse_integer(const Lexer::Token& token);
    double parse_float(const Lexer::Token& token);

    Value done(const Lexer::Token& token);
};

} //json

#endif //JSON_PARSER_H
