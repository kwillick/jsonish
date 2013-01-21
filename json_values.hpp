#ifndef JSON_VALUE_H
#define JSON_VALUE_H

#include <vector>
#include <map>
#include <memory>
#include <string>

namespace json
{

class Value;

enum class e_JsonType
{
    object = 0,
    array,
    string,
    integer,
    floating_point,
    true_value,
    false_value,
    null_value
};

class String
{
  public:
    String(const char* start, const char* end) : m_start(start), m_end(end) { }

    bool operator<(const String& rhs) const
    { return std::lexicographical_compare(m_start, m_end, rhs.m_start, rhs.m_end); }

    std::string to_string() const { return std::string(m_start, m_end); }
    
  private:
    const char* m_start;
    const char* m_end;
};

typedef std::map<String, std::unique_ptr<Value>> Object;
typedef std::vector<std::unique_ptr<Value>>      Array;

namespace impl
{

template <e_JsonType J>
struct result_type;

} //impl

class Value
{
  public:
    Value();

    Value(const Object& obj);
    Value(Object&& obj);
    
    Value(const Array& arr);
    Value(Array&& arr);
    
    Value(const String& str);

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
    typename impl::result_type<J>::type& get(typename impl::result_type<J>::type* = nullptr);

    template <e_JsonType J>
    const typename impl::result_type<J>::type&
    get(const typename impl::result_type<J>::type* = nullptr) const;

  private:
    e_JsonType m_type;
    union
    {
        Object m_object;
        Array m_array;
        String m_string;
        long long m_integer;
        double m_floating_point;
    };

    void copy_guts(const Value& o);
    void move_guts(Value&& o);
    

#define GET_IMPL(t, n)                                  \
    t& get_impl(t*) { return n; }                       \
    const t& get_impl(const t*) const { return n; }

    GET_IMPL(Object, m_object)
    GET_IMPL(Array, m_array)
    GET_IMPL(String, m_string)
    GET_IMPL(long long, m_integer)
    GET_IMPL(double, m_floating_point)

#undef GET_IMPL
};


namespace impl
{

#define RESULT_IMPL(e, t) template <> struct result_type<e> { typedef t type; }

RESULT_IMPL(e_JsonType::object,         Object);
RESULT_IMPL(e_JsonType::array,          Array);
RESULT_IMPL(e_JsonType::string,         String);
RESULT_IMPL(e_JsonType::integer,        long long);
RESULT_IMPL(e_JsonType::floating_point, double);

#undef RESULT_IMPL

} //impl

template <e_JsonType J>
typename impl::result_type<J>::type& Value::get(typename impl::result_type<J>::type* p)
{ return get_impl(p); }

template <e_JsonType J>
const typename impl::result_type<J>::type& 
Value::get(const typename impl::result_type<J>::type* p) const
{ return get_impl(p); }

} //json

#endif //JSON_VALUE_H
