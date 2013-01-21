#include "json_value.hpp"
#include <utility>

namespace json
{

Value::Value() : m_type(e_JsonType::null_value) { }

Value::Value(bool b) : m_type(b ? e_JsonType::true_value : e_JsonType::false_value) { }

void Value::copy_guts(const Value& o)
{
    switch (m_type)
    {
    case e_JsonType::object:
        new (&m_object) Object(o.m_object);
        break;
    case e_JsonType::array:
        new (&m_array) Array(o.m_array);
        break;
    case e_JsonType::string:
        new (&m_string) String(o.m_string);
        break;
    case e_JsonType::integer:
        m_integer = o.m_integer;
        break;
    case e_JsonType::floating_point:
        m_floating_point = o.m_floating_point;
        break;
    default:
        break;
    }
}

void Value::move_guts(Value&& o)
{
    switch (m_type)
    {
    case e_JsonType::object:
        new (&m_object) Object(std::forward<Object>(o.m_object));
        break;
    case e_JsonType::array:
        new (&m_array) Array(std::forward<Array>(o.m_array));
        break;
    case e_JsonType::string:
        new (&m_string) String(std::forward<String>(o.m_string));
        break;
    case e_JsonType::integer:
        m_integer = o.m_integer;
        break;
    case e_JsonType::floating_point:
        m_floating_point = o.m_floating_point;
        break;
    default:
        break;
    }
    //invalidate old value
    o.m_type = e_JsonType::null_value; 
}

Value::Value(const Value& o) : m_type(o.m_type) { copy_guts(o); }
Value::Value(Value&& o) : m_type(o.m_type) { move_guts(std::forward<Value>(o)); }

Value& Value::operator=(const Value& o)
{
    if (this != &o)
        copy_guts(o);
    return *this;
}

Value& Value::operator=(Value&& o)
{
    m_type = o.m_type;
    move_guts(std::forward<Value>(o));
}

Value::~Value()
{
    switch (m_type)
    {
    case e_JsonType::object: m_object.~Object();        break;
    case e_JsonType::array:  m_array.~Array();          break;
    default:                                            break;
    }
}

} //json
