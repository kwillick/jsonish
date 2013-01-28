#include "json_object.hpp"

namespace json
{

Object::Object() { }

Object::~Object() { }

Value& Object::operator[](const char* str)
{
    auto pos = std::lower_bound(m_pairs.begin(),
                                m_pairs.end(),
                                str,
                                [](const Member& a, const char* b) -> bool {
                                    return a.first < b;
                                });
    return pos->second;
}

const Value& Object::operator[](const char* str) const
{
    auto pos = std::lower_bound(m_pairs.cbegin(),
                                m_pairs.cend(),
                                str,
                                [](const Member& a, const char* b) -> bool {
                                    return a.first < b;
                                });
    return pos->second;
}

Value& Object::operator[](const std::string& str)
{
    auto pos = std::lower_bound(m_pairs.begin(),
                                m_pairs.end(),
                                str,
                                [](const Member& a, const std::string& b) -> bool {
                                    return a.first < b;
                                });
    return pos->second;
}

const Value& Object::operator[](const std::string& str) const
{
    auto pos = std::lower_bound(m_pairs.cbegin(),
                                m_pairs.cend(),
                                str,
                                [](const Member& a, const std::string& b) -> bool {
                                    return a.first < b;
                                });
    return pos->second;
}

} //json
