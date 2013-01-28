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

    bool operator<(const String& rhs) const
    {
        auto len = std::min(std::distance(m_start, m_end), std::distance(rhs.m_start, rhs.m_end));
        return strncmp(m_start, rhs.m_start, len) < 0;
    }

    bool operator<(const char* rhs) const
    {
        auto dist = std::distance(m_start, m_end);
        auto len = std::min(dist, static_cast<decltype(dist)>(strlen(rhs)));
        return strncmp(m_start, rhs, len) < 0;
    }

    bool operator<(const std::string& rhs) const
    {
        auto dist = std::distance(m_start, m_end);
        auto len = std::min(dist, static_cast<decltype(dist)>(rhs.length()));
        return strncmp(m_start, rhs.data(), len) < 0;
    }

    std::string to_string() const { return std::string(m_start, m_end); }
    
  private:
    const char* m_start;
    const char* m_end;
};

} //json

#endif //JSON_STRING_H
