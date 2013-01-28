#include "json_parser.hpp"
#include <cstring>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <iterator>

#include "json_object.hpp"

namespace json
{

enum class e_LexerError
{
    UnknownCharacter = 0,
    UnterminatedString,
    UnexpectedEnd,
    ExpectedTrue,
    ExpectedFalse,
    ExpectedNull,
    BadNumber,
    Count
};

static const char* s_lexer_errors[static_cast<unsigned>(e_LexerError::Count)] =
{
    "Unknown Character",
    "Unterminated string",
    "Unexpected end of input",
    "Expected 'true'",
    "Expected 'false'",
    "Expected 'null'",
    "Malformed number"
};

Lexer::Lexer(const char* start, const char* end)
    : m_pos(start),
      m_end(end)
{
}

Lexer::Token Lexer::next()
{
    while (m_pos != m_end)
    {
        auto start = m_pos;
        char c = *m_pos++;
        switch (c)
        {
            //whitespace
        case ' ':
        case '\n':
        case '\t':
        case '\r':
            break;
            
            //symbols
        case '{':
            return Token(e_Token::LeftBrace, start, m_pos);
        case '}':
            return Token(e_Token::RightBrace, start, m_pos);
        case '[':
            return Token(e_Token::LeftBracket, start, m_pos);
        case ']':
            return Token(e_Token::RightBracket, start, m_pos);
        case ':':
            return Token(e_Token::Colon, start, m_pos);
        case ',':
            return Token(e_Token::Comma, start, m_pos);

            //string
        case '"':
            return read_string();

            //Numbers
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return read_number();

            //true, false, null
        case 't':
            return read_potential_true();
        case 'f':
            return read_potential_false();
        case 'n':
            return read_potential_null();

        default:
            return Token(m_pos,
                         s_lexer_errors[static_cast<unsigned>(e_LexerError::UnknownCharacter)]);
        }
    }

    return Token(e_Token::EndOfInput, nullptr, nullptr);
}

Lexer::Token Lexer::read_string()
{
    auto start = m_pos;
    while (m_pos != m_end);
    {
        if (*m_pos++ == '"')
        {
            //the quote has been skipped
            return Token(e_Token::String, start, m_pos - 1);
        }
    } 

    return Token(start, s_lexer_errors[static_cast<unsigned>(e_LexerError::UnterminatedString)]);
}

Lexer::Token Lexer::read_number()
{
    auto start = m_pos - 1;
    bool is_fp = false;
    while (m_pos != m_end)
    {
        if (*m_pos == '.')
            is_fp = true;
        else if (!std::isspace(*m_pos))
        {
            auto t = is_fp ? e_Token::Float : e_Token::Integer;
            return Token(t, start, m_pos);
        }

        m_pos++;
    }

    return Token(start, s_lexer_errors[static_cast<unsigned>(e_LexerError::UnexpectedEnd)]);
}

Lexer::Token Lexer::read_potential_true()
{
    //rue
    auto start = m_pos - 1;
    if ((m_pos != m_end) && (*m_pos++ == 'r') &&
        (m_pos != m_end) && (*m_pos++ == 'u') &&
        (m_pos != m_end) && (*m_pos++ == 'e'))
    {
        return Token(e_Token::True, nullptr, nullptr);
    }

    return Token(start, s_lexer_errors[static_cast<unsigned>(e_LexerError::ExpectedTrue)]);
}

Lexer::Token Lexer::read_potential_false()
{
    //alse
    auto start = m_pos - 1;
    if ((m_pos != m_end) && (*m_pos++ == 'a') &&
        (m_pos != m_end) && (*m_pos++ == 'l') &&
        (m_pos != m_end) && (*m_pos++ == 's') &&
        (m_pos != m_end) && (*m_end++ == 'e'))
    {
        return Token(e_Token::False, nullptr, nullptr);
    }

    return Token(start, s_lexer_errors[static_cast<unsigned>(e_LexerError::ExpectedFalse)]);
}

Lexer::Token Lexer::read_potential_null()
{
    //ull
    auto start = m_pos - 1;
    if ((m_pos != m_end) && (*m_pos++ == 'u') &&
        (m_pos != m_end) && (*m_pos++ == 'l') &&
        (m_pos != m_end) && (*m_pos++ == 'l'))
    {
        return Token(e_Token::Null, nullptr, nullptr);
    }

    return Token(start, s_lexer_errors[static_cast<unsigned>(e_LexerError::ExpectedNull)]);
}

Parser::Parser(const char* input) : Parser(input, input + strlen(input) + 1)
{
}

Parser::Parser(const char* start, const char* end)
    : m_start(start),
      m_end(end),
      m_lexer(start, end)
{
}

/*
  need to distinguish reduced object / array from unfinished ones

  Value = Object, Array, String, Integer, Float, true, false, null
  (8)

  Token     | top of stack | action
  ----------------------------------
     {      | empty        | push Object 1
     {      | Value        | push Object 8
     
     }      | empty        | Error 1
     }      | Value        | pop until Object at top, make pairs 8
     
     [      | empty        | push Array 1
     [      | Value        | push Array 8
     
     ]      | empty        | Error 1
     ]      | Value        | pop until Array at top, make Array 8
     
     :      | empty        | Error 1
     :      | String       | continue 1
     :      | other        | Error 7

     ,      | Value        | continue 8
     ,      | empty        | Error 1

     String | Value        | push String 8
     String | empty        | Error 1

     Int    | Value        | push Int 8
     Int    | empty        | Error 1

     Float  | Value        | push Number 8
     Float  | empty        | Error 1
     
     true   | Value        | push true 8
     true   | empty        | Error 1

     false  | Value        | push false 8
     false  | empty        | Error 1

     null   | Value        | push null 8
     null   | empty        | Error 1

     EOI    | Object       | Done 1
     EOI    | Array        | Done 1
     EOI    | empty        | Error 1
     EOI    | other        | Error 6

     Error  | anything     | Error 9
 */

enum class e_Action : uint8_t
{
    Push, Pop, Continue, Error, Done
};

//first index is e_Token and second is empty combined with e_JsonType
static const e_Action s_state_table[14][9] =
{
    //e_Token::LeftBrace
    {
        e_Action::Push, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::RightBrace
    {
        e_Action::Error, //empty
        e_Action::Pop, //e_JsonType::Object
        e_Action::Pop, //e_JsonType::Array
        e_Action::Pop, //e_JsonType::String
        e_Action::Pop, //e_JsonType::Integer
        e_Action::Pop, //e_JsonType::FloatingPoint
        e_Action::Pop, //e_JsonType::True
        e_Action::Pop, //e_JsonType::False
        e_Action::Pop  //e_JsonType::Null
    },

    //e_Token::LeftBracket
    {
        e_Action::Push, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::RightBracket
    {
        e_Action::Error, //empty
        e_Action::Pop, //e_JsonType::Object
        e_Action::Pop, //e_JsonType::Array
        e_Action::Pop, //e_JsonType::String
        e_Action::Pop, //e_JsonType::Integer
        e_Action::Pop, //e_JsonType::FloatingPoint
        e_Action::Pop, //e_JsonType::True
        e_Action::Pop, //e_JsonType::False
        e_Action::Pop  //e_JsonType::Null
    },

    //e_Token::Colon
    {
        e_Action::Error, //empty
        e_Action::Error, //e_JsonType::Object
        e_Action::Error, //e_JsonType::Array
        e_Action::Continue, //e_JsonType::String
        e_Action::Error, //e_JsonType::Integer
        e_Action::Error, //e_JsonType::FloatingPoint
        e_Action::Error, //e_JsonType::True
        e_Action::Error, //e_JsonType::False
        e_Action::Error  //e_JsonType::Null
    },

    //e_Token::Comma
    {
        e_Action::Error, //empty
        e_Action::Continue, //e_JsonType::Object
        e_Action::Continue, //e_JsonType::Array
        e_Action::Continue, //e_JsonType::String
        e_Action::Continue, //e_JsonType::Integer
        e_Action::Continue, //e_JsonType::FloatingPoint
        e_Action::Continue, //e_JsonType::True
        e_Action::Continue, //e_JsonType::False
        e_Action::Continue  //e_JsonType::Null
    },

    //e_Token::String
    {
        e_Action::Error, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::Integer
    {
        e_Action::Error, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::Float
    {
        e_Action::Error, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::True
    {
        e_Action::Error, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::False
    {
        e_Action::Error, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::Null
    {
        e_Action::Error, //empty
        e_Action::Push, //e_JsonType::Object
        e_Action::Push, //e_JsonType::Array
        e_Action::Push, //e_JsonType::String
        e_Action::Push, //e_JsonType::Integer
        e_Action::Push, //e_JsonType::FloatingPoint
        e_Action::Push, //e_JsonType::True
        e_Action::Push, //e_JsonType::False
        e_Action::Push  //e_JsonType::Null
    },

    //e_Token::EndOfInput
    {
        e_Action::Error, //empty
        e_Action::Done, //e_JsonType::Object
        e_Action::Done, //e_JsonType::Array
        e_Action::Error, //e_JsonType::String
        e_Action::Error, //e_JsonType::Integer
        e_Action::Error, //e_JsonType::FloatingPoint
        e_Action::Error, //e_JsonType::True
        e_Action::Error, //e_JsonType::False
        e_Action::Error  //e_JsonType::Null
    },

    //e_Token::Error
    {
        e_Action::Error, //empty
        e_Action::Error, //e_JsonType::Object
        e_Action::Error, //e_JsonType::Array
        e_Action::Error, //e_JsonType::String
        e_Action::Error, //e_JsonType::Integer
        e_Action::Error, //e_JsonType::FloatingPoint
        e_Action::Error, //e_JsonType::True
        e_Action::Error, //e_JsonType::False
        e_Action::Error  //e_JsonType::Null
    }
};

void Parser::reset()
{
    m_lexer = Lexer(m_start, m_end);
    m_stack.clear();
}

void Parser::reset(const char* input)
{
    m_start = input;
    m_end = m_start + strlen(input) + 1;
    reset();
}

void Parser::reset(const char* start, const char* end)
{
    m_start = start;
    m_end = end;
    reset();
}

Value Parser::parse(std::function<void(const Parser::Error&)> error_fun)
{
    try
    {
        while (true)
        {
            auto token = m_lexer.next();
            const auto& action = s_state_table[static_cast<unsigned int>(token.type)][top_type()];

            switch (action)
            {
            case e_Action::Push:     push(token);       break;
            case e_Action::Pop:      pop(token);        break;
            case e_Action::Continue:                    break;
            case e_Action::Error:    error(token);      break;
            case e_Action::Done:                        return done(token);
            }
        }
    }
    catch (const Error& err)
    {
        error_fun(err);
        return Value();
    }
}

//this is then second index into s_state_table
unsigned int Parser::top_type() const
{
    if (m_stack.empty())
        return 0;
    
    const auto& top = m_stack.front();
    return static_cast<unsigned int>(top.type()) + 1;
}

void Parser::push(const Lexer::Token& token)
{
    //these should be e_Tokens
    switch (token.type)
    {
    case e_Token::LeftBrace:
        m_stack.emplace_front(Object());
        break;
    case e_Token::LeftBracket:
        m_stack.emplace_front(Array());
        break;
    case e_Token::String:
        m_stack.emplace_front(String(token.value.start, token.value.end));
        break;
    case e_Token::Integer:
        m_stack.emplace_front(parse_integer(token));
        break;
    case e_Token::Float:
        m_stack.emplace_front(parse_float(token));
        break;
    case e_Token::True:
        m_stack.emplace_front(Value(true));
        break;
    case e_Token::False:
        m_stack.emplace_front(Value(false));
        break;
    case e_Token::Null:
        m_stack.emplace_front(Value());
        break;
    default:
        throw Error(token.value.start, nullptr);
    }
}

void Parser::pop(const Lexer::Token& token)
{    
    auto it = m_stack.begin();
    if (token.type == e_Token::RightBracket)
    {
        for (; it != m_stack.end(); ++it)
        {
            if (it->type() == e_JsonType::Array)
            {
                const Array& array = it->get<e_JsonType::Array>();
                if (array.empty())
                    break;
            } 
        }
    }
    else if (token.type == e_Token::RightBrace)
    {
        for (; it != m_stack.end(); ++it)
        {
            if (it->type() == e_JsonType::Object)
            {
                const Object& obj = it->get<e_JsonType::Object>();
                if (obj.empty())
                    break;
            }
        }
    }

    if (it == m_stack.end())
        throw Error(token.value.start, nullptr);

    auto start = m_stack.begin();

    typedef std::move_iterator<decltype(it)> move_iter_type;
    if (it->type() == e_JsonType::Array)
    {
        auto& array = it->get<e_JsonType::Array>();
        array.reserve(std::distance(start, it));
        array.assign(move_iter_type(start), move_iter_type(it));
    }
    else if (it->type() == e_JsonType::Object)
    {
        auto& object = it->get<e_JsonType::Object>();
        object.move_assign(start, it);
    }

    m_stack.erase(start, it);
}

void Parser::error(const Lexer::Token& token)
{
    if (token.type == e_Token::Error)
        throw Error(token.error.pos, token.error.message);
    else
        throw Error(token.value.start, nullptr);
}

long long Parser::parse_integer(const Lexer::Token& token)
{
    char* endptr = nullptr;
    long long result = strtoll(token.value.start, &endptr, 10);
    if (endptr != token.value.end)
        throw Error(token.value.start, nullptr); //invalid integer

    if (errno == ERANGE)
        throw Error(token.value.start, nullptr); //integer overflow/underflow

    return result;
}

double Parser::parse_float(const Lexer::Token& token)
{
    char* endptr = nullptr;
    double result = strtod(token.value.start, &endptr);
    if (endptr != token.value.end)
        throw Error(token.value.start, nullptr); //invalid float

    if (errno == ERANGE)
        throw Error(token.value.start, nullptr); //float overflow/underflow

    return result;
}

Value Parser::done(const Lexer::Token& token)
{
    if (token.type != e_Token::EndOfInput)
        throw Error(token.value.start, nullptr);

    if (m_stack.size() != 1)
        throw Error(token.value.start, nullptr);

    std::move_iterator<decltype(m_stack.begin())> move_it(m_stack.begin());
    Value result(*move_it);
    m_stack.pop_front();

    return result;
}

} //json
