#include "json_writer.hpp"
#include <utility>
#include <stack>
#include <cstdint>

namespace json
{

namespace writer
{

Object::Object() { }

Object::Object(std::initializer_list<std::pair<const std::string, Value>> ilist)
    : m_pairs(ilist) { }


Object::const_iterator Object::cbegin() const { return m_pairs.cbegin(); }
Object::const_iterator Object::cend() const { return m_pairs.cend(); }

Object::~Object() { }


Value::Value() : m_type{e_JsonType::Null} { }

Value::Value(const Object& obj) : m_type{e_JsonType::Object}, m_object{new Object(obj)} { }

Value::Value(Object&& obj) 
    : m_type{e_JsonType::Object}, 
      m_object{new Object(std::forward<Object>(obj))}
{
}

Value::Value(const Array& arr) : m_type{e_JsonType::Array}, m_array{new Array(arr)} { }

Value::Value(Array&& arr) : 
    m_type{e_JsonType::Array},
    m_array{new Array(std::forward<Array>(arr))}
{
}

Value::Value(const std::string& str) : m_type{e_JsonType::String}, m_string{str} { }

Value::Value(int i) : m_type{e_JsonType::Integer}, m_integer{i} { }

Value::Value(long long i) : m_type{e_JsonType::Integer}, m_integer{i} { }

Value::Value(double d) : m_type{e_JsonType::FloatingPoint}, m_floating_point{d} { }

Value::Value(bool b) : m_type{b ? e_JsonType::True : e_JsonType::False} { }

void Value::copy_guts(const Value& o)
{
    switch (m_type)
    {
    case e_JsonType::Object:
        m_object = new Object(*o.m_object);
        break;
    case e_JsonType::Array:
        m_array = new Array(*o.m_array);
        break;
    case e_JsonType::String:
        new (&m_string) std::string(o.m_string);
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
        new (&m_string) std::string(std::move(o.m_string));
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

enum class e_StackElementType : uint8_t
{
    Object,
    Array
};

template <typename Iter>
struct range
{
    Iter start;
    Iter end;
    Iter pos;
};

struct stack_value
{
    typedef Object::const_iterator object_iterator;
    typedef Array::const_iterator  array_iterator;

    e_StackElementType type;
    union
    {
        range<Object::const_iterator> object_pos;
        range<Array::const_iterator> array_pos;
    };

    stack_value(object_iterator s, object_iterator e, object_iterator p)
        : type{e_StackElementType::Object},
          object_pos{s, e, p} 
    { }

    stack_value(array_iterator s, array_iterator e, array_iterator p)
        : type{e_StackElementType::Array},
          array_pos{s, e, p} 
    { }

    stack_value(const stack_value& o) : type{o.type}
    {
        if (type == e_StackElementType::Object)
            object_pos = o.object_pos;
        else
            array_pos = o.array_pos;
    }

    stack_value(stack_value&& o) : type{o.type}
    {
        if (type == e_StackElementType::Object)
            object_pos = std::move(o.object_pos);
        else
            array_pos = std::move(o.array_pos);
    }

    stack_value& operator=(const stack_value& o)
    {
        type = o.type;
        if (type == e_StackElementType::Object)
            object_pos = o.object_pos;
        else
            array_pos = o.array_pos;
        return *this;
    }

    stack_value& operator=(stack_value&& o)
    {
        type = o.type;
        if (type == e_StackElementType::Object)
            object_pos = std::move(o.object_pos);
        else
            array_pos = std::move(o.array_pos);
        return *this;
    }
};

static inline void simple_value_write(std::ostream& o, const Value& v)
{
    switch (v.type())
    {
    default: break;

    case e_JsonType::String:
        o << '"' << v.get<e_JsonType::String>() << '"';
        break;
    case e_JsonType::Integer:
        o << v.get<e_JsonType::Integer>();
        break;
    case e_JsonType::FloatingPoint:
        o << v.get<e_JsonType::FloatingPoint>();
        break;
    case e_JsonType::True:
        o << "true";
        break;
    case e_JsonType::False:
        o << "false";
        break;
    case e_JsonType::Null:
        o << "null";
        break;
    }
}

static inline void write_comma_on_close(std::ostream& o, const std::stack<stack_value>& stack)
{
    if (!stack.empty())
    {
        const auto& top = stack.top();
        if (top.type == e_StackElementType::Object)
        {
            if (top.object_pos.start != top.object_pos.end &&
                top.object_pos.pos != top.object_pos.end)
            {
                o << ',';
            }
        }
        else
        {
            if (top.array_pos.start != top.array_pos.end &&
                top.array_pos.pos != top.array_pos.end)
            {
                o << ',';
            }
        }
    }
}

static void write_impl(std::ostream& o, std::stack<stack_value>& stack)
{
    while (!stack.empty())
    {
    loop_begin:
        stack_value top{stack.top()};
        stack.pop();
        if (top.type == e_StackElementType::Object)
        {
            if (top.object_pos.start == top.object_pos.end)
            {
                o << "{}";
                write_comma_on_close(o, stack);
                continue;
            }
            
            if (top.object_pos.pos == top.object_pos.start)
                o << '{';

            auto end = top.object_pos.end;
            auto last_comma = end;
            --last_comma;

            for (auto& pos = top.object_pos.pos; pos != end; ++pos)
            {
                o << '"' << pos->first << "\":";
                switch (pos->second.type())
                {
                case e_JsonType::Object: 
                    {
                        auto next = pos;
                        ++next;
                        stack.emplace(top.object_pos.start, end, next);

                        const auto& obj = pos->second.get<e_JsonType::Object>();
                        stack.emplace(obj.cbegin(), obj.cend(), obj.cbegin());
                        goto loop_begin;
                    }
                    break;

                case e_JsonType::Array:
                    {
                        auto next = pos;
                        ++next;
                        stack.emplace(top.object_pos.start, end, next);

                        const auto& arr = pos->second.get<e_JsonType::Array>();
                        stack.emplace(arr.cbegin(), arr.cend(), arr.cbegin());
                        goto loop_begin;
                    }
                    break;

                default: 
                    simple_value_write(o, pos->second);
                    break;
                }

                if (pos != last_comma)
                    o << ',';
            }
            
            o << '}';
            write_comma_on_close(o, stack);
        }
        else
        {
            if (top.array_pos.start == top.array_pos.end)
            {
                o << "[]";
                write_comma_on_close(o, stack);
                continue;
            }

            if (top.array_pos.pos == top.array_pos.start)
                o << '[';
            
            auto end = top.array_pos.end;
            auto last_comma = end;
            --last_comma;

            for (auto& pos = top.array_pos.pos; pos != end; ++pos)
            {
                switch (pos->type())
                {
                case e_JsonType::Object:
                    {
                        auto next = pos;
                        ++next;
                        stack.emplace(top.array_pos.start, end, next);
                        
                        const auto& obj = pos->get<e_JsonType::Object>();
                        stack.emplace(obj.cbegin(), obj.cend(), obj.cbegin());
                        goto loop_begin;
                    }
                    break;
                case e_JsonType::Array:
                    {
                        auto next = pos;
                        ++next;
                        stack.emplace(top.array_pos.start, end, next);
                        
                        const auto& arr = pos->get<e_JsonType::Array>();
                        stack.emplace(arr.cbegin(), arr.cend(), arr.cbegin());
                        goto loop_begin;
                    }
                    break;

                default:
                    simple_value_write(o, *pos);
                    break;
                }

                if (pos != last_comma)
                    o << ',';
            }
            
            o << ']';
            write_comma_on_close(o, stack);
        }
    }
}

void write(std::ostream& o, const Object& object)
{
    std::stack<stack_value> stack;
    stack.emplace(object.cbegin(), object.cend(), object.cbegin());

    write_impl(o, stack);
}

void write(std::ostream& o, const Array& array)
{
    std::stack<stack_value> stack;
    stack.emplace(array.cbegin(), array.cend(), array.cbegin());
    
    write_impl(o, stack);
}

} //writer

} //json
