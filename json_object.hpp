#ifndef JSON_OBJECT_H
#define JSON_OBJECT_H

#include <utility>
#include <map>
#include <iterator>
#include <algorithm>
#include <string>
#include <cstring>
#include <initializer_list>

#include "json_string.hpp"
#include "json_value.hpp"

namespace json
{

class Object
{
  public:
    typedef std::map<String, Value>::iterator       iterator;
    typedef std::map<String, Value>::const_iterator const_iterator;
    
    Object() { }
    Object(std::initializer_list<std::pair<const String, Value>> ilist) : m_pairs(ilist) { }

    ~Object() { }

    template <typename InputIter, typename GetFunc>
    void move_assign(InputIter start, InputIter end, GetFunc f);

    Value& operator[](const char* str)
    {
        return m_pairs[String(str, str + std::strlen(str))];
    }

    const Value& operator[](const char* str) const
    {
        String key{str, str + std::strlen(str)};
        auto pos = m_pairs.find(key);
        return pos->second;
    }

    Value& operator[](const std::string& str)
    {
        auto start = &str[0];
        return m_pairs[String(start, start + str.length())];
    }

    const Value& operator[](const std::string& str) const
    {
        auto start = &str[0];
        String key{start, start + str.length()};
        auto pos = m_pairs.find(key);
        return pos->second;
    }

    iterator begin()              { return m_pairs.begin(); }
    const_iterator begin() const  { return m_pairs.cbegin(); }

    iterator end()                { return m_pairs.end(); }
    const_iterator end() const    { return m_pairs.cend(); }

    const_iterator cbegin() const { return m_pairs.cbegin(); }
    const_iterator cend() const { return m_pairs.cend(); }

    bool empty() const            { return m_pairs.empty(); }
    std::size_t size() const      { return m_pairs.size(); }

  private:
    std::map<String, Value> m_pairs;
};

template <typename InputIter, typename GetFunc>
void Object::move_assign(InputIter start, InputIter end, GetFunc f)
{
    //iterator expects (value, key)
    m_pairs.clear();
    
    for (; start != end; ++start)
    {
        Value first{ std::forward<Value>(f(*start++)) };
        Value second{ std::forward<Value>(f(*start)) };

        m_pairs.emplace(second.get<e_JsonType::String>(), first);
    }
}

} //json

#endif //JSON_OBJECT_H
