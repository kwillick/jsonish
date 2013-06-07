#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include <ostream>
#include <stack>
#include <algorithm>
#include <iterator>
#include "json_object.hpp"
#include "json_value.hpp"

namespace json
{

void write(std::ostream& o, const Object& object);
void write(std::ostream& o, const Array& array);

template <unsigned int IndentWidth = 4>
void write_pretty(std::ostream& o, const Object& object);

template <unsigned int IndentWidth = 4>
void write_pretty(std::ostream& o, const Array& object);


namespace impl
{

template <typename Iter>
struct range
{
    Iter start;
    Iter end;
    Iter pos;
};

enum class e_StackElementType : uint8_t
{
    Object,
    Array
};

struct stack_value
{
    using object_iterator = Object::const_iterator;
    using array_iterator = Array::const_iterator;

    using object_range = range<object_iterator>;
    using array_range = range<array_iterator>;

    e_StackElementType type;
    union
    {
        object_range object_pos;
        array_range array_pos;
    };

    unsigned int depth;

    stack_value(object_iterator s, object_iterator e, object_iterator p, unsigned int d)
        : type{e_StackElementType::Object},
          object_pos{s, e, p},
          depth{d} { }

    stack_value(array_iterator s, array_iterator e, array_iterator p, unsigned int d)
        : type{e_StackElementType::Array},
          array_pos{s, e, p},
          depth{d} { }

    stack_value(const stack_value& o) : type{o.type}, depth{o.depth}
    {
        if (type == e_StackElementType::Object)
            object_pos = o.object_pos;
        else
            array_pos = o.array_pos;
    }

    stack_value(stack_value&& o) : type{o.type}, depth{o.depth}
    {
        if (type == e_StackElementType::Object)
            object_pos = std::move(o.object_pos);
        else
            array_pos = std::move(o.array_pos);
    }

    stack_value& operator=(const stack_value& o)
    {
        type = o.type;
        depth = o.depth;
        if (type == e_StackElementType::Object)
            object_pos = o.object_pos;
        else
            array_pos = o.array_pos;
        return *this;
    }

    stack_value& operator=(stack_value&& o)
    {
        type = o.type;
        depth = o.depth;
        if (type == e_StackElementType::Object)
            object_pos = std::move(o.object_pos);
        else
            array_pos = std::move(o.array_pos);
        return *this;
    }

    bool is_object() const       { return type == e_StackElementType::Object; }

    bool object_empty() const    { return object_pos.start == object_pos.end; }
    bool array_empty() const     { return array_pos.start == array_pos.end; }

    bool at_object_start() const { return object_pos.pos == object_pos.start; }
    bool at_array_start() const  { return array_pos.pos == array_pos.start; }

    bool at_object_end() const   { return object_pos.pos == object_pos.end; }
    bool at_array_end() const    { return array_pos.pos == array_pos.end; }
};


template <std::size_t N>
static inline void ostream_write(std::ostream& o, const char (&str)[N]) { o.write(str, N - 1); }


template <unsigned int IndentWidth>
struct indenter
{
    static void indent(std::ostream& o, int n)
    {
        std::fill_n(std::ostream_iterator<char>(o), IndentWidth * n, ' ');
    }

    static void object_open(std::ostream& o) { ostream_write(o, "{\n"); }
    static void object_close(std::ostream& o, int n) 
    {
        o.put('\n');
        indent(o, n - 1);
        o.put('}');
    }

    static void array_open(std::ostream& o) { ostream_write(o, "[\n"); }
    static void array_close(std::ostream& o, int n)
    {
        o.put('\n');
        indent(o, n - 1);
        o.put(']');
    }

    static void comma(std::ostream& o, const stack_value& top)
    {
        if ((top.is_object() && !top.object_empty() && !top.at_object_end()) ||
            (!top.array_empty() && !top.at_array_end()))
            o.write(",\n", 2);
    }

    static void comma(std::ostream& o)       { ostream_write(o, ",\n"); }
    static void colon(std::ostream& o)       { ostream_write(o, ": "); }
    static void space(std::ostream& o)       { o.put(' '); }
};

template <>
struct indenter<0>
{
    static void indent(std::ostream& o, int n) { }
    static void object_open(std::ostream& o) { o.put('{'); }
    static void object_close(std::ostream& o, int n) { o.put('}'); }
    static void array_open(std::ostream& o) { o.put('['); }
    static void array_close(std::ostream& o, int n) { o.put(']'); }

    static void comma(std::ostream& o, const stack_value& top)
    {
        if ((top.is_object() && !top.object_empty() && !top.at_object_end()) ||
            (!top.array_empty() && !top.at_array_end()))
            o.put(',');
    }

    static void comma(std::ostream& o) { o.put(','); }
    static void colon(std::ostream& o) { o.put(':'); }
    static void space(std::ostream& o) { }
};

static inline void write_string(std::ostream& o, const String& s)
{
    o.put('"');
    auto len = std::distance(s.begin(), s.end());
    o.write(s.begin(), len);
    o.put('"');
}

static inline void write_simple_value(std::ostream& o, const Value& v)
{
    switch (v.type())
    {
    default: break;

    case e_JsonType::String:
        write_string(o, v.get<e_JsonType::String>());
        break;
    case e_JsonType::Integer:
        o << v.get<e_JsonType::Integer>();
        break;
    case e_JsonType::FloatingPoint:
        o << v.get<e_JsonType::FloatingPoint>();
        break;
    case e_JsonType::True:
        ostream_write(o, "true");
        break;
    case e_JsonType::False:
        ostream_write(o, "false");
        break;
    case e_JsonType::Null:
        ostream_write(o,"null");
        break;
    }
}

template <unsigned int IndentWidth, typename T>
inline void write(std::ostream& o, const T& val)
{
    using indent_type = indenter<IndentWidth>;

    std::stack<stack_value> stack;
    stack.emplace(val.cbegin(), val.cend(), val.cbegin(), 1);

    auto push_object_state = [&stack](stack_value::object_iterator pos, const stack_value& top)
    {
        stack.emplace(top.object_pos.start,
                      top.object_pos.end,
                      ++pos,
                      top.depth);
    };

    auto push_array_state = [&stack](stack_value::array_iterator pos, const stack_value& top)
    {
        stack.emplace(top.array_pos.start,
                      top.array_pos.end,
                      ++pos,
                      top.depth);
    };

    while (!stack.empty())
    {
    loop_begin:
        stack_value top{stack.top()};
        stack.pop();
        if (top.is_object())
        {
            if (top.object_empty())
            {
                //{}
                indent_type::object_open(o);
                indent_type::object_close(o, top.depth);

                if (!stack.empty())
                    indent_type::comma(o, stack.top());
                
                continue;
            }

            if (top.at_object_start())
            {
                //{
                indent_type::object_open(o);
            }

            auto last_comma = top.object_pos.end;
            --last_comma;

            for (auto& pos = top.object_pos.pos; pos != top.object_pos.end; ++pos)
            {
                //string
                indent_type::indent(o, top.depth);
                write_string(o, pos->first);
                indent_type::colon(o);

                //value
                switch (pos->second.type())
                {
                case e_JsonType::Object:
                    {
                        push_object_state(pos, top);

                        const auto& obj = pos->second.get<e_JsonType::Object>();
                        stack.emplace(obj.cbegin(), obj.cend(), obj.cbegin(), top.depth + 1);
                        goto loop_begin;
                    }
                    break;

                case e_JsonType::Array:
                    {
                        push_object_state(pos, top);

                        const auto& arr = pos->second.get<e_JsonType::Array>();
                        stack.emplace(arr.cbegin(), arr.cend(), arr.cbegin(), top.depth + 1);
                        goto loop_begin;
                    }
                    break;

                default:
                    write_simple_value(o, pos->second);
                    break;
                }

                if (pos != last_comma)
                    indent_type::comma(o);
            }

            indent_type::object_close(o, top.depth);

            if (!stack.empty())
                indent_type::comma(o, stack.top());
        }
        else
        {
            if (top.array_empty())
            {
                //[]
                indent_type::array_open(o);
                indent_type::array_close(o, top.depth);

                if (!stack.empty())
                    indent_type::comma(o, stack.top());

                continue;
            }

            if (top.at_array_start())
            {
                //[
                indent_type::array_open(o);
            }

            auto last_comma = top.array_pos.end;
            --last_comma;

            for (auto& pos = top.array_pos.pos; pos != top.array_pos.end; ++pos)
            {
                indent_type::indent(o, top.depth);
                switch (pos->type())
                {
                case e_JsonType::Object:
                    {
                        push_array_state(pos, top);

                        const auto& obj = pos->get<e_JsonType::Object>();
                        stack.emplace(obj.cbegin(), obj.cend(), obj.cbegin(), top.depth + 1);
                        goto loop_begin;
                    }
                    break;
                    
                case e_JsonType::Array:
                    {
                        push_array_state(pos, top);

                        const auto& arr = pos->get<e_JsonType::Array>();
                        stack.emplace(arr.cbegin(), arr.cend(), arr.cbegin(), top.depth + 1);
                        goto loop_begin;
                    }
                    break;

                default:
                    write_simple_value(o, *pos);
                    break;
                }

                if (pos != last_comma)
                    indent_type::comma(o);
            }

            indent_type::array_close(o, top.depth);

            if (!stack.empty())
                indent_type::comma(o, stack.top());
        }
    }
}

} //impl


void write(std::ostream& o, const Object& object)
{
    impl::write<0>(o, object);
}

void write(std::ostream& o, const Array& array)
{
    impl::write<0>(o, array);
}


template <unsigned int IndentWidth>
void write_pretty(std::ostream& o, const Object& object)
{
    impl::write<IndentWidth>(o, object);
}

template <unsigned int IndentWidth>
void write_pretty(std::ostream& o, const Array& array)
{
    impl::write<IndentWidth>(o, array);
}

} //json

#endif //JSON_WRITER_H
