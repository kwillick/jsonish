#ifndef JSON_OBJECT_H
#define JSON_OBJECT_H

#include <vector>
#include <iterator>
#include <type_traits>
#include <algorithm>
#include <utility>
#include <string>

#include "json_string.hpp"
#include "json_value.hpp"

namespace json
{

class Object
{
  public:
    typedef std::pair<String, Value>            Member;
    typedef std::vector<Member>::iterator       iterator;
    typedef std::vector<Member>::const_iterator const_iterator;
    
    Object();
    ~Object();

    template <typename InputIter, typename GetFunc>
    void move_assign(InputIter start, InputIter end, GetFunc f);

    Value& operator[](const char* str);
    const Value& operator[](const char* str) const;

    Value& operator[](const std::string& str);
    const Value& operator[](const std::string& str) const;

    iterator begin()             { return m_pairs.begin(); }
    const_iterator begin() const { return m_pairs.cbegin(); }

    iterator end()               { return m_pairs.end(); }
    const_iterator end() const   { return m_pairs.cend(); }

    bool empty() const           { return m_pairs.empty(); }
    std::size_t size() const     { return m_pairs.size(); }

  private:
    std::vector<Member> m_pairs;
};

template <typename InputIter, typename GetFunc>
void Object::move_assign(InputIter start, InputIter end, GetFunc f)
{
    //iterator expects (value, key)
    m_pairs.clear();
    
    m_pairs.reserve(std::distance(start, end));
    for (; start != end; ++start)
    {
        Value first{ std::forward<Value>(f(*start++)) };
        Value second{ std::forward<Value>(f(*start)) };

        m_pairs.emplace_back(second.get<e_JsonType::String>(), first);
    }

    std::sort(m_pairs.begin(),
              m_pairs.end(),
              [](const Member& a, const Member& b) -> bool {
                  return a.first < b.first;
              });
}

} //json

#endif //JSON_OBJECT_H
