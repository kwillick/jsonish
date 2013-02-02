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
    typedef std::pair<String, Value>            Member;
  public:
    typedef std::vector<Member>::iterator       iterator;
    typedef std::vector<Member>::const_iterator const_iterator;
    
    Object();
    ~Object();

    template <typename InputIter>
    void move_assign(InputIter start, InputIter end);

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
    std::vector<Member> m_pairs; //should I be using a std::map<String, Value> ?
};

template <typename InputIter>
void Object::move_assign(InputIter start, InputIter end)
{
    //iterator expecst (value, key)

    static_assert(std::is_same<typename std::iterator_traits<InputIter>::value_type, Value>::value,
                  "Iterator must have a value_type of json::Value");

    typedef std::move_iterator<InputIter> move_iterator;
    
    m_pairs.clear();
    
    m_pairs.reserve(std::distance(start, end));
    for (; start != end; ++start)
    {
        move_iterator first(start++);
        move_iterator second(start);

        Value s(*second);
        m_pairs.emplace_back(s.take<e_JsonType::String>(), std::forward<Value>(*first));
    }

    std::sort(m_pairs.begin(),
              m_pairs.end(),
              [](const Member& a, const Member& b) -> bool {
                  return a.first < b.first;
              });
}

} //json

#endif //JSON_OBJECT_H
