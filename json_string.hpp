#ifndef JSON_STRING_H
#define JSON_STRING_H

#include <cstring>
#include <algorithm>

namespace json
{

class String
{
  public:
    String(const char* start, const char* end) : m_start(start), m_end(end) { }

    bool operator<(const String& rhs) const
    {
        auto len = std::min(m_end - m_start, rhs.m_end - rhs.m_start);
        return strncmp(m_start, rhs.m_start, len) < 0;
    }

    std::string to_string() const { return std::string(m_start, m_end); }
    
  private:
    const char* m_start;
    const char* m_end;
};

} //json

#endif //JSON_STRING_H
