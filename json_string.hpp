#ifndef JSON_STRING_H
#define JSON_STRING_H

#include <cstring>
#include <string>
#include <algorithm>
#include <iterator>

namespace json
{

class String
{
  public:
    String(const char* start, const char* end) : m_start(start), m_end(end) { }

    template <std::size_t N>
    String(const char (&str)[N]) : m_start(str), m_end(str + N - 1) { }

    bool operator<(const String& rhs) const
    {
        return std::lexicographical_compare(m_start, m_end, rhs.m_start, rhs.m_end);
    }

    bool operator<(const char* rhs) const
    {
        return std::lexicographical_compare(m_start, m_end, rhs, rhs + std::strlen(rhs) + 1);
    }

    bool operator<(const std::string& rhs) const
    {
        return std::lexicographical_compare(m_start, m_end, rhs.cbegin(), rhs.cend());
    }

    std::string to_string() const { return std::string(m_start, m_end); }

    const char* begin() const { return m_start; }
    const char* end() const { return m_end; }
    
  private:
    const char* m_start;
    const char* m_end;
};

} //json

#endif //JSON_STRING_H
