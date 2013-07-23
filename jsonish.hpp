#ifndef JSONISH_H
#define JSONISH_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <ostream>
#include <stack>
#include <string>
#include <vector>

namespace jsonish
{

class String
{
  public:
    String(const char* start, const char* end) : m_start(start), m_end(end) { }

    template <std::size_t N>
    String(const char (&str)[N]) : m_start(str), m_end(str + N - 1) { }

    bool operator<(const String& rhs) const
    {
        return std::lexicographical_compare(m_start, m_end, rhs.m_start, rhs.m_end);
    }

    bool operator<(const char* rhs) const
    {
        return std::lexicographical_compare(m_start, m_end, rhs, rhs + std::strlen(rhs) + 1);
    }

    bool operator<(const std::string& rhs) const
    {
        return std::lexicographical_compare(m_start, m_end, rhs.cbegin(), rhs.cend());
    }

    std::string to_string() const { return std::string(m_start, m_end); }

    const char* begin() const { return m_start; }
    const char* end() const { return m_end; }
    
  private:
    const char* m_start;
    const char* m_end;
};


enum class e_JsonType
{
    Object = 0,
    Array,
    String,
    Integer,
    FloatingPoint,
    True,
    False,
    Null
};


namespace impl
{

template <e_JsonType J>
struct result_type;

} //impl

class Value;
class Object;

typedef std::vector<Value> Array;


class Value
{
  public:
    Value();

    Value(const Object& obj);
    Value(Object&& obj);
    
    Value(const Array& arr);
    Value(Array&& arr);
    
    Value(const String& str);

    Value(int i);
    Value(long long i);
    Value(double d);
    Value(bool b);

    Value(const Value& o);
    Value(Value&& o);
    Value& operator=(const Value& o);
    Value& operator=(Value&& o);

    ~Value();

    e_JsonType type() const { return m_type; }

    template <e_JsonType J>
    typename impl::result_type<J>::type& get(typename impl::result_type<J>::type* unused = nullptr)
    { return get_impl(unused); }

    template <e_JsonType J>
    const typename impl::result_type<J>::type&
    get(const typename impl::result_type<J>::type* unused = nullptr) const
    { return get_impl(unused); }

  private:
    e_JsonType m_type;
    union
    {
        Object* m_object;
        Array* m_array;
        String m_string;
        long long m_integer;
        double m_floating_point;
    };

    void copy_guts(const Value& o);
    void move_guts(Value&& o);
    

#define GET_IMPL(t, n)                                          \
    inline t& get_impl(t*) { return n; }                        \
    inline const t& get_impl(const t*) const { return n; }

    GET_IMPL(Object,    *m_object)
    GET_IMPL(Array,     *m_array)
    GET_IMPL(String,    m_string)
    GET_IMPL(long long, m_integer)
    GET_IMPL(double,    m_floating_point)

#undef GET_IMPL
};


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
        return m_pairs[String{start, start + str.length()}];
    }

    const Value& operator[](const std::string& str) const
    {
        auto start = &str[0];
        String key{start, start + str.length()};
        auto pos = m_pairs.find(key);
        return pos->second;
    }

    iterator find(const char* key)
    {
        return m_pairs.find(String{key, key + std::strlen(key)});
    }

    iterator find(const std::string& key)
    {
        auto start = &key[0];
        return m_pairs.find(String{start, start + key.length()});
    }

    const_iterator find(const char* key) const
    {
        return m_pairs.find(String{key, key + std::strlen(key)});
    }

    const_iterator find(const std::string& key) const
    {
        auto start = &key[0];
        return m_pairs.find(String{start, start + key.length()});
    }

    iterator begin()              { return m_pairs.begin(); }
    const_iterator begin() const  { return m_pairs.cbegin(); }

    iterator end()                { return m_pairs.end(); }
    const_iterator end() const    { return m_pairs.cend(); }

    const_iterator cbegin() const { return m_pairs.cbegin(); }
    const_iterator cend() const   { return m_pairs.cend(); }

    bool empty() const            { return m_pairs.empty(); }
    std::size_t size() const      { return m_pairs.size(); }

  private:
    std::map<String, Value> m_pairs;
};


//Value impl details
namespace impl
{

#define RESULT_IMPL(e, t) template <> struct result_type<e> { typedef t type; }

RESULT_IMPL(e_JsonType::Object,        Object);
RESULT_IMPL(e_JsonType::Array,         Array);
RESULT_IMPL(e_JsonType::String,        String);
RESULT_IMPL(e_JsonType::Integer,       long long);
RESULT_IMPL(e_JsonType::FloatingPoint, double);

#undef RESULT_IMPL

} //impl

//Object impl details
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


enum class e_Token : uint8_t
{
    LeftBrace = 0,
    RightBrace,
    LeftBracket,
    RightBracket,
    Colon,
    Comma,
    String,
    Integer,
    Float,
    True,
    False,
    Null,
    EndOfInput,
    Error
};

class Lexer
{
  public:
    struct TokenValue
    {
        const char* start;
        const char* end;
    };

    struct TokenError
    {
        const char* pos;
        const char* message;
    };
    
    struct Token
    {     
        e_Token type;
        union
        {
            TokenValue value;
            TokenError error;
        };

        Token() : type(e_Token::Error), value{ nullptr, nullptr } { }
        
        Token(e_Token t, const char* s, const char* e) : type(t), value{ s, e }
        {
        }

        Token(const char* s, const char* e) : type(e_Token::Error), error{ s, e }
        {
        }
    };

    Lexer() = delete;

    Lexer(const char* start, const char* end);

    Token next();
    Token peek();

  private:
    const char* m_pos;
    const char* m_end;

    Token read_string();
    Token read_number();
    Token read_potential_true();
    Token read_potential_false();
    Token read_potential_null();
};

struct Error
{
    const char* pos;
    const char* message;
    Error() : pos(nullptr), message(nullptr) { }
    Error(const char* p, const char* m) : pos(p), message(m) { }
};

class Parser
{
  public:
    Parser() = delete;

    Parser(const char* input);
    Parser(const char* start, const char* end);
    Parser(const std::string& input);

    void reset();
    void reset(const char* input);
    void reset(const char* start, const char* end);
    void reset(const std::string& input);

    Value parse(std::function<void(const Error&)> error_fun);

  private:
    const char* m_start;
    const char* m_end;
    Lexer m_lexer;

    enum class e_Expect : uint8_t
    {
        Value = 0,
        ValueOrClose,
        CommaOrClose,
        StringOrClose,
        String,
        Colon,
        EndOfInput
    };
    e_Expect m_expect;
    
    enum class e_Context : uint8_t
    {
        None,
        Object,
        Array
    };
    e_Context m_context;

    unsigned int m_length;

    struct stack_state
    {
        Value value;
        e_Context context;
        unsigned int length;

        stack_state(Value&& v, e_Context c, unsigned int l) 
            : value(std::forward<Value>(v)), context(c), length(l) { }
    };

    std::deque<stack_state> m_stack;

    unsigned int top_type() const;
    void check_expect(const Lexer::Token& token) const;
    void push(const Lexer::Token& token);
    void pop(const Lexer::Token& token);
    void pop_until_object(const Lexer::Token& token);
    void pop_until_array(const Lexer::Token& token);
    void comma_colon(const Lexer::Token& token);
    void error(const Lexer::Token& token);

    long long parse_integer(const Lexer::Token& token);
    double parse_float(const Lexer::Token& token);

    Value done(const Lexer::Token& token);
};


//output

void write(std::ostream& o, const Value& val);

template <unsigned int IndentWidth = 4>
void write_pretty(std::ostream& o, const Value& val);


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
inline void ostream_write(std::ostream& o, const char (&str)[N]) { o.write(str, N - 1); }


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
            ostream_write(o, ",\n");
    }

    static void comma(std::ostream& o)       { ostream_write(o, ",\n"); }
    static void colon(std::ostream& o)       { ostream_write(o, ": "); }
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
};

inline void write_string(std::ostream& o, const String& s)
{
    o.put('"');
    auto len = std::distance(s.begin(), s.end());
    o.write(s.begin(), len);
    o.put('"');
}

inline void write_simple_value(std::ostream& o, const Value& v)
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
        {
            char buf[std::numeric_limits<double>::max_digits10 + 1];
            double d = v.get<e_JsonType::FloatingPoint>();
            std::snprintf(buf, std::numeric_limits<double>::max_digits10 + 1, "%f", d);
            o << buf;
        break;
        }
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

template <unsigned int IndentWidth>
void write_pretty(std::ostream& o, const Value& val)
{
    switch (val.type())
    {
    case e_JsonType::Object:
        impl::write<IndentWidth>(o, val.get<e_JsonType::Object>());
        break;
    case e_JsonType::Array:
        impl::write<IndentWidth>(o, val.get<e_JsonType::Array>());
        break;
    default:
        impl::write_simple_value(o, val);
        break;
    }
}

} //jsonish

#endif //JSONISH_H
