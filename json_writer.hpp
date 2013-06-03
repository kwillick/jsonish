#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include <string>
#include <vector>
#include <map>
#include <initializer_list>
#include <ostream>
#include <cstdio>

namespace json
{

namespace writer
{

class Value;

class Object
{
  public:
    typedef std::map<std::string, Value> map_type;
    typedef map_type::const_iterator     const_iterator;

    Object();
    Object(std::initializer_list<std::pair<const std::string, Value>> ilist);
    ~Object();

    const_iterator cbegin() const;
    const_iterator cend() const;

  private:
    std::map<std::string, Value> m_pairs;
};

typedef std::vector<Value>           Array;

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

class Value
{
  public:
    Value();

    Value(const Object& obj);
    Value(Object&& obj);
    
    Value(const Array& arr);
    Value(Array&& arr);

    Value(const std::string& str);

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
    typename impl::result_type<J>::type& get(typename impl::result_type<J>::type* = nullptr);

    template <e_JsonType J>
    const typename impl::result_type<J>::type&
    get(const typename impl::result_type<J>::type* = nullptr) const;

  private:
    e_JsonType m_type;

    union
    {
        Object* m_object;
        Array* m_array;
        std::string m_string;
        long long m_integer;
        double m_floating_point;
    };

    void copy_guts(const Value& o);
    void move_guts(Value&& o);

    
#define GET_IMPL(t, n)                                          \
    inline t& get_impl(t*) { return n; }                        \
    inline const t& get_impl(const t*) const { return n; }

    GET_IMPL(Object,      *m_object)
    GET_IMPL(Array,       *m_array)
    GET_IMPL(std::string, m_string)
    GET_IMPL(long long,   m_integer)
    GET_IMPL(double,      m_floating_point)

#undef GET_IMPL

};


void write(std::ostream& o, const Object& object);
void write(std::ostream& o, const Array& array);


namespace impl
{

#define RESULT_IMPL(e, t) template <> struct result_type<e> { typedef t type; }

RESULT_IMPL(e_JsonType::Object,        Object);
RESULT_IMPL(e_JsonType::Array,         Array);
RESULT_IMPL(e_JsonType::String,        std::string);
RESULT_IMPL(e_JsonType::Integer,       long long);
RESULT_IMPL(e_JsonType::FloatingPoint, double);

#undef RESULT_IMPL

} //impl

template <e_JsonType J>
typename impl::result_type<J>::type& Value::get(typename impl::result_type<J>::type* p)
{ return get_impl(p); }

template <e_JsonType J>
const typename impl::result_type<J>::type& 
Value::get(const typename impl::result_type<J>::type* p) const
{ return get_impl(p); }

} //writer

} //json

#endif //JSON_WRITER_H
