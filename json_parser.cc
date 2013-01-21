#include "json_parser.hpp"
#include <cstring>
#include <cctype>

namespace json
{

enum class e_LexerError
{
    UnknownCharacter = 0,
    UnterminatedString,
    ExpectedTrue,
    ExpectedFalse,
    ExpectedNull,
    BadNumber,
    Count
};

static const char* s_lexer_errors[e_LexerError::Count] =
{
    "Unknown Character",
    "Unterminated string",
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
            return read_number(c == '-');

            //true, false, null
        case 't':
            return read_potential_true();
        case 'f':
            return read_potential_false();
        case 'n':
            return read_potential_null();

        default:
            return Token(pos, s_lexer_errors[e_LexerError::UnknownCharacters]);
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

    return Token(start, s_lexer_errors[e_LexerError::UnterminatedString]);
}

Lexer::Token Lexer::read_number(bool minus)
{
    auto start = minus ? m_pos - 1 : m_pos;
    
}

} //json
