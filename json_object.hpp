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

//TODO: switch Member to a std::pair
class Object
{
  public:
    struct Member
    {
        String key;
        Value value;
    };

    typedef std::vector<Member>::iterator       iterator;
    typedef std::vector<Member>::const_iterator const_iterator;
    
    Object() { }
    ~Object() { }

    template <typename InputIter>
    void assign(InputIter start, InputIter end);

    Value& operator[](const String& s);
    const Value& operator[](const String& s) const;

    iterator begin()             { return m_pairs.begin(); }
    const_iterator begin() const { return m_pairs.cbegin(); }

    iterator end()               { return m_pairs.end(); }
    const_iterator end() const   { return m_pairs.cend(); }

    bool empty() const           { return m_pairs.empty(); }
    std::size_t size() const     { return m_pairs.size(); }

  private:
    std::vector<Member> m_pairs;
};

template <typename InputIter>
void Object::assign(InputIter start, InputIter end)
{
    m_pairs.clear();
    
    m_pairs.reserve(std::distance(start, end));
    for (; start != end; ++start)
        m_pairs.emplace_back({*start++, *start });

    std::sort(m_pairs.begin(),
              m_pairs.end(),
              [](const Member& a, const Member& b) -> bool {
                  return a.key < b.key;
              });
}

Value& Object::operator[](const String& s)
{
    auto pos = std::lower_bound(m_pairs.begin(),
                                m_pairs.end(), 
                                s, 
                                [](const Member& a, const String& b) -> bool {
                                    return a.key < b;
                                });
    return pos->value;
}

const Value& Object::operator[](const String& s) const
{
    auto pos = std::lower_bound(m_pairs.cbegin(),
                                m_pairs.cend(), 
                                s, 
                                [](const Member& a, const String& b) -> bool {
                                    return a.key < b;
                                });
    return pos->value;
}

} //json

#endif //JSON_OBJECT_H
