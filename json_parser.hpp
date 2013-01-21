#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <deque>
#include <utility>
#include "json_values.hpp"

namespace json
{

enum class e_Token
{
    LeftBrace,
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
    Token read_number(bool minus);
    Token read_potential_true();
    Token read_potential_false();
    Token read_potential_null();
};

class Parser
{
  public:
    Parser() = delete;

    Parser(const char* input);
    Parser(const char* start, const char* end);

    Value parse();

  private:
    Lexer m_lexer;

    typedef std::pair<e_Token, Value> parse_state;
    std::deque<parse_state> m_stack;
};

} //json

#endif //JSON_PARSER_H
