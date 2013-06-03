#include "json_value.hpp"
#include "json_object.hpp"
#include <utility>

namespace json
{

Value::Value() : m_type{e_JsonType::Null} { }

Value::Value(const Object& obj) : m_type{e_JsonType::Object}, m_object{new Object{obj}} { }

Value::Value(Object&& obj) 
    : m_type{e_JsonType::Object}, 
      m_object{new Object(std::forward<Object>(obj))}
{
}

Value::Value(const Array& arr) : m_type{e_JsonType::Array}, m_array{new Array{arr}} { }

Value::Value(Array&& arr) : 
    m_type{e_JsonType::Array},
    m_array{new Array(std::forward<Array>(arr))}
{
}

Value::Value(const String& str) : m_type{e_JsonType::String}, m_string{str} { }

Value::Value(long long i) : m_type{e_JsonType::Integer}, m_integer{i} { }

Value::Value(double d) : m_type{e_JsonType::FloatingPoint}, m_floating_point{d} { }

Value::Value(bool b) : m_type{b ? e_JsonType::True : e_JsonType::False} { }

void Value::copy_guts(const Value& o)
{
    switch (m_type)
    {
    case e_JsonType::Object:
        m_object = new Object{*o.m_object};
        break;
    case e_JsonType::Array:
        m_array = new Array{*o.m_array};
        break;
    case e_JsonType::String:
        new (&m_string) String{o.m_string};
        break;
    case e_JsonType::Integer:
        m_integer = o.m_integer;
        break;
    case e_JsonType::FloatingPoint:
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
    case e_JsonType::Object:
        m_object = o.m_object;
        o.m_object = nullptr;
        break;
    case e_JsonType::Array:
        m_array = o.m_array;
        o.m_array = nullptr;
        break;
    case e_JsonType::String:
        new (&m_string) String(std::move(o.m_string));
        break;
    case e_JsonType::Integer:
        m_integer = o.m_integer;
        break;
    case e_JsonType::FloatingPoint:
        m_floating_point = o.m_floating_point;
        break;
    default:
        break;
    }
    //invalidate old value
    o.m_type = e_JsonType::Null;
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
    return *this;
}

Value::~Value()
{
    switch (m_type)
    {
    case e_JsonType::Object: delete m_object;       break;
    case e_JsonType::Array:  delete m_array;        break;
    default:                                        break;
    }
}

} //json
