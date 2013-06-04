#include "json_parser.hpp"
#include <cstring>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <iterator>
#include <type_traits>
#include <cmath>
#include <algorithm>
#include "json_object.hpp"


namespace json
{

template <typename T>
constexpr typename std::underlying_type<T>::type enum_value(T val)
{ return static_cast<typename std::underlying_type<T>::type>(val); }

enum class e_LexerError : uint8_t
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

static const char* s_lexer_errors[enum_value(e_LexerError::Count)] =
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
                         s_lexer_errors[enum_value(e_LexerError::UnknownCharacter)]);
        }
    }

    return Token(e_Token::EndOfInput, nullptr, nullptr);
}

Lexer::Token Lexer::peek()
{
    auto start = m_pos;
    auto result = next();
    m_pos = start;

    return result;
}

Lexer::Token Lexer::read_string()
{
    auto start = m_pos;
    while (m_pos != m_end)
    {
        if (*m_pos++ == '"')
        {
            //the quote has been skipped
            return Token(e_Token::String, start, m_pos - 1);
        }
    } 

    return Token(start, s_lexer_errors[enum_value(e_LexerError::UnterminatedString)]);
}

Lexer::Token Lexer::read_number()
{
    auto start = m_pos - 1;
    bool is_fp = false;
    while (m_pos != m_end)
    {
        if (*m_pos == '.')
        {
            if (*start == '-' && m_pos == start + 1)
                return Token(start, s_lexer_errors[enum_value(e_LexerError::BadNumber)]);
            is_fp = true;
        }
        else if (!std::isdigit(*m_pos))
        {
            auto t = is_fp ? e_Token::Float : e_Token::Integer;
            return Token(t, start, m_pos);
        }

        m_pos++;
    }

    return Token(start, s_lexer_errors[enum_value(e_LexerError::UnexpectedEnd)]);
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

    return Token(start, s_lexer_errors[enum_value(e_LexerError::ExpectedTrue)]);
}

Lexer::Token Lexer::read_potential_false()
{
    //alse
    auto start = m_pos - 1;
    if ((m_pos != m_end) && (*m_pos++ == 'a') &&
        (m_pos != m_end) && (*m_pos++ == 'l') &&
        (m_pos != m_end) && (*m_pos++ == 's') &&
        (m_pos != m_end) && (*m_pos++ == 'e'))
    {
        return Token(e_Token::False, nullptr, nullptr);
    }

    return Token(start, s_lexer_errors[enum_value(e_LexerError::ExpectedFalse)]);
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

    return Token(start, s_lexer_errors[enum_value(e_LexerError::ExpectedNull)]);
}

Parser::Parser(const char* input) : Parser(input, input + strlen(input) + 1)
{
}

Parser::Parser(const char* start, const char* end)
    : m_start(start),
      m_end(end),
      m_lexer(start, end),
      m_expect(e_Expect::Value),
      m_context(e_Context::None),
      m_length(0)
{
}

Parser::Parser(const std::string& input) : Parser(input.data(), input.data() + input.length())
{
}


enum class e_ParseError : uint8_t
{
    UnclosedObject = 0,
    UnclosedArray,
    TopLevelNotObjectOrArray,
    ExpectedString,
    ExpectedStringOrCloseObject,
    ExpectedColon,
    ExpectedCommaOrCloseObject,
    ExpectedCommaOrCloseArray,
    ExpectedValue,
    ExpectedEndOfInput,
    IntegerOverflow,
    IntegerUnderflow,
    FloatingPointOverflow,
    FloatingPointUnderflow,
    Count
};

static const char* s_parse_errors[enum_value(e_ParseError::Count)] =
{
    "Unclosed Object",
    "Unclosed Array",
    "Top level must be an Object or an Array",
    "Expected string",
    "Expected string or '}'",
    "Expected ':'",
    "Expected ',' or '}'",
    "Expected ',' or ']'",
    "Expected object, array, string, number, true, false, or null",
    "Expected end of input",
    "Integer overflow",
    "Integer underflow",
    "Floating point overflow",
    "Floating point underflow"
};

/*
  Token     | top of stack | action
  ----------------------------------
     {      | empty        | push Object
     {      | Value        | push Object
     
     }      | empty        | Error
     }      | Value        | pop until Object at top, make pairs
     
     [      | empty        | push Array
     [      | Value        | push Array
     
     ]      | empty        | Error
     ]      | Value        | pop until Array at top, make Array
     
     :      | empty        | Error
     :      | String       | continue
     :      | other        | Error

     ,      | Value        | continue
     ,      | empty        | Error

     String | Value        | push String
     String | empty        | Error

     Int    | Value        | push Int
     Int    | empty        | Error

     Float  | Value        | push Number
     Float  | empty        | Error
     
     true   | Value        | push true
     true   | empty        | Error

     false  | Value        | push false
     false  | empty        | Error

     null   | Value        | push null
     null   | empty        | Error

     EOI    | Object       | Done
     EOI    | Array        | Done
     EOI    | empty        | Error
     EOI    | other        | Error

     Error  | anything     | Error
 */

enum class e_Action : uint8_t
{
    Push, Pop, Continue, Error, Done
};

/*
  First index is an e_Token.
  Second represents the type of the top of the stack as a e_JsonType combined with the empty state
*/
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

void Parser::reset(const std::string& input)
{
    m_start = input.data();
    m_end = input.data() + input.length();
    reset();
}

Value Parser::parse(std::function<void(const Error&)> error_fun)
{
    try
    {
        auto peek = m_lexer.peek();
        if (peek.type != e_Token::LeftBrace && peek.type != e_Token::LeftBracket)
        {
            throw Error(peek.value.start, 
                        s_parse_errors[enum_value(e_ParseError::TopLevelNotObjectOrArray)]);
        }

        m_expect = e_Expect::Value;
        
        while (true)
        {
            auto token = m_lexer.next();
            const auto& action = s_state_table[enum_value(token.type)][top_type()];
            
            switch (action)
            {
            case e_Action::Push:     push(token);          break;
            case e_Action::Pop:      pop(token);           break;
            case e_Action::Continue: comma_colon(token);   break;
            case e_Action::Error:    error(token);         break;
            case e_Action::Done:     return done(token);
            }
        }
    }
    catch (const Error& err)
    {
        error_fun(err);
        return Value();
    }
}

//this is the second index into s_state_table
unsigned int Parser::top_type() const
{
    if (m_stack.empty())
        return 0;
    
    const auto& top = m_stack.front();
    return static_cast<unsigned int>(top.value.type()) + 1;
}

static inline bool _token_is_value(const Lexer::Token& token)
{
    return (token.type == e_Token::LeftBrace   ||
            token.type == e_Token::LeftBracket ||
            token.type == e_Token::String      ||
            token.type == e_Token::Integer     ||
            token.type == e_Token::Float       ||
            token.type == e_Token::True        ||
            token.type == e_Token::False       ||
            token.type == e_Token::Null);
}

void Parser::check_expect(const Lexer::Token& token) const
{
    switch (m_expect)
    {
    case e_Expect::Value:
        if (!_token_is_value(token))
            throw Error(token.value.start, s_parse_errors[enum_value(e_ParseError::ExpectedValue)]);
        break;
    case e_Expect::ValueOrClose:
        if (!_token_is_value(token) && 
            token.type != e_Token::RightBrace &&
            token.type != e_Token::RightBracket)
        {
            throw Error(token.value.start, s_parse_errors[enum_value(e_ParseError::ExpectedValue)]);
        }
        break;
    case e_Expect::CommaOrClose:
        {
            const char* emsg_ptr = nullptr;
        
            if (token.type != e_Token::Comma &&
                token.type != e_Token::RightBrace &&
                token.type != e_Token::RightBracket)
            {
                if (m_context == e_Context::Object)
                    emsg_ptr = s_parse_errors[enum_value(e_ParseError::ExpectedCommaOrCloseObject)];
                else if (m_context == e_Context::Array)
                    emsg_ptr = s_parse_errors[enum_value(e_ParseError::ExpectedCommaOrCloseArray)];
                throw Error(token.value.start, emsg_ptr);
            }

            if (token.type == e_Token::RightBrace && m_context == e_Context::Array)
                emsg_ptr = s_parse_errors[enum_value(e_ParseError::ExpectedCommaOrCloseArray)];
            else if (token.type == e_Token::RightBracket && m_context == e_Context::Object)
                emsg_ptr = s_parse_errors[enum_value(e_ParseError::ExpectedCommaOrCloseObject)];

            if (emsg_ptr)
                throw Error(token.value.start, emsg_ptr);
        }
        break;
    case e_Expect::StringOrClose:
        if (token.type != e_Token::String &&
            token.type != e_Token::RightBrace &&
            token.type != e_Token::RightBracket)
        {
            throw Error(token.value.start,
                        s_parse_errors[enum_value(e_ParseError::ExpectedStringOrCloseObject)]);
        }
        break;
    case e_Expect::String:
        if (token.type != e_Token::String)
        {
            throw Error(token.value.start,
                        s_parse_errors[enum_value(e_ParseError::ExpectedString)]);
        }
        break;
    case e_Expect::Colon:
        if (token.type != e_Token::Colon)
            throw Error(token.value.start, s_parse_errors[enum_value(e_ParseError::ExpectedColon)]);
        break;
    case e_Expect::EndOfInput:
        if (token.type != e_Token::EndOfInput)
        {
            throw Error(token.value.start,
                        s_parse_errors[enum_value(e_ParseError::ExpectedEndOfInput)]);
        }
        break;
    }
}

void Parser::push(const Lexer::Token& token)
{
    check_expect(token);

    switch (token.type)
    {
    case e_Token::LeftBrace:
        m_stack.emplace_front(Object(), m_context, m_length);
        m_context = e_Context::Object;
        m_expect = e_Expect::StringOrClose;
        m_length = 0;
        break;
    case e_Token::LeftBracket:
        m_stack.emplace_front(Array(), m_context, m_length);
        m_context = e_Context::Array;
        m_expect = e_Expect::ValueOrClose;
        m_length = 0;
        break;
    case e_Token::String:
        if (m_context == e_Context::Object)
        {
            if (m_expect == e_Expect::Value)
                m_expect = e_Expect::CommaOrClose;
            else if (m_expect == e_Expect::String || m_expect == e_Expect::StringOrClose)
                m_expect = e_Expect::Colon;
        }
        else if (m_context == e_Context::Array)
            m_expect = e_Expect::CommaOrClose;

        m_stack.emplace_front(String(token.value.start, token.value.end), m_context, m_length);
        m_length++;
        break;
    case e_Token::Integer:
        m_stack.emplace_front(parse_integer(token), m_context, m_length);
        m_expect = e_Expect::CommaOrClose;
        m_length++;
        break;
    case e_Token::Float:
        m_stack.emplace_front(parse_float(token), m_context, m_length);
        m_expect = e_Expect::CommaOrClose;
        m_length++;
        break;
    case e_Token::True:
        m_stack.emplace_front(Value(true), m_context, m_length);
        m_expect = e_Expect::CommaOrClose;
        m_length++;
        break;
    case e_Token::False:
        m_stack.emplace_front(Value(false), m_context, m_length);
        m_expect = e_Expect::CommaOrClose;
        m_length++;
        break;
    case e_Token::Null:
        m_stack.emplace_front(Value(), m_context, m_length);
        m_expect = e_Expect::CommaOrClose;
        m_length++;
        break;
    default:
        throw Error(token.value.start, nullptr);
    }
}

void Parser::pop(const Lexer::Token& token)
{
    check_expect(token);

    if (token.type == e_Token::RightBracket)
        pop_until_array(token);
    else if (token.type == e_Token::RightBrace)
        pop_until_object(token);

    if (m_context != e_Context::None)
        m_expect = e_Expect::CommaOrClose;
    else
        m_expect = e_Expect::EndOfInput;
}

void Parser::pop_until_object(const Lexer::Token& token)
{
    auto start = m_stack.begin();
    auto it = start + m_length;

    if (start != it)
    {
        auto& object = it->value.get<e_JsonType::Object>();
        object.move_assign(start, it,
                           [](stack_state& v) -> Value&& {
                               return std::move(v.value);
                           });
        m_stack.erase(start, it);
    }

    m_context = it->context;
    m_length = it->length + 1;
}

template <typename InputIter, typename OutputIter, typename UnaryOp>
static inline OutputIter move_transform(InputIter start, InputIter end, OutputIter out, UnaryOp op)
{
    using value_type = typename OutputIter::value_type;
    while (start != end)
        *out++ = std::forward<value_type>(op(*start++));
    return out;
}

void Parser::pop_until_array(const Lexer::Token& token)
{
    auto start = m_stack.begin();
    auto it = start + m_length;

    using reverse_iter_type = std::reverse_iterator<decltype(it)>;

    if (start != it)
    {
        auto& array = it->value.get<e_JsonType::Array>();
        array.resize(std::distance(start, it));

        reverse_iter_type rstart{ start };
        reverse_iter_type rit{ it };

        move_transform(rit, rstart, array.begin(),
                       [](stack_state& v) -> Value&& {
                           return std::move(v.value);
                       });

        m_stack.erase(start, it);
    }

    m_context = it->context;
    m_length = it->length + 1;
}

void Parser::comma_colon(const Lexer::Token& token)
{
    check_expect(token);

    if (m_context == e_Context::Object && token.type == e_Token::Comma)
        m_expect = e_Expect::String;
    else
        m_expect = e_Expect::Value;
}

void Parser::error(const Lexer::Token& token)
{
    switch (token.type)
    {
    case e_Token::Error:
        throw Error(token.error.pos, token.error.message);
    case e_Token::EndOfInput:
        {
            const char* emsg_ptr = nullptr;
            if (m_context == e_Context::Object)
                emsg_ptr = s_parse_errors[enum_value(e_ParseError::UnclosedObject)];
            else if (m_context == e_Context::Array)
                emsg_ptr = s_parse_errors[enum_value(e_ParseError::UnclosedArray)];
            throw Error(token.value.start, emsg_ptr);
        }
    default:
        throw Error(token.value.start, nullptr);
    }
}

long long Parser::parse_integer(const Lexer::Token& token)
{
    //leading zero
    if (std::distance(token.value.start, token.value.end) > 1 && *token.value.start == '0')
        throw Error(token.value.start, s_lexer_errors[enum_value(e_LexerError::BadNumber)]);
    
    char* endptr = nullptr;
    long long result = strtoll(token.value.start, &endptr, 10);
    if (endptr != token.value.end)
        throw Error(token.value.start, s_lexer_errors[enum_value(e_LexerError::BadNumber)]);

    if (errno == ERANGE)
    {
        const char* emsg_ptr = nullptr;
        if (result == LLONG_MIN)
            emsg_ptr = s_parse_errors[enum_value(e_ParseError::IntegerUnderflow)];
        else if (result == LLONG_MAX)
            emsg_ptr = s_parse_errors[enum_value(e_ParseError::IntegerOverflow)];
        throw Error(token.value.start, emsg_ptr);
    }

    return result;
}

double Parser::parse_float(const Lexer::Token& token)
{
    char* endptr = nullptr;
    double result = strtod(token.value.start, &endptr);
    if (endptr != token.value.end)
        throw Error(token.value.start, s_lexer_errors[enum_value(e_LexerError::BadNumber)]);

    if (errno == ERANGE)
    {
        const char* emsg_ptr = nullptr;
        if (result == HUGE_VAL || result == -HUGE_VAL)
            emsg_ptr = s_parse_errors[enum_value(e_ParseError::FloatingPointOverflow)];
        else if (result == 0)
            emsg_ptr = s_parse_errors[enum_value(e_ParseError::FloatingPointUnderflow)];

        throw Error(token.value.start, emsg_ptr);
    }

    return result;
}

Value Parser::done(const Lexer::Token& token)
{
    if (token.type != e_Token::EndOfInput)
    {
        throw Error(token.value.start,
                    s_parse_errors[enum_value(e_ParseError::ExpectedEndOfInput)]);
    }

    if (m_stack.size() != 1)
    {
        throw Error(token.value.start,
                    s_parse_errors[enum_value(e_ParseError::ExpectedEndOfInput)]);
    }

    auto start = m_stack.begin();
    Value result(std::move(start->value));
    m_stack.pop_front();

    return result;
}

} //json
