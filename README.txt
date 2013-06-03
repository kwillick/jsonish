JSON-ish

A JSON-ish parser using C++11.

Why the "ish"?
This is an almost conforming JSON parser. It doesn't handle strings correctly 
according to the spec. It will accept anything at all between quotes. This was 
written mainly as a learning experience in writting a parser modeled after push
down automata.


A basic example
int main(int argc, char* argv[])
{
    std::string json_text = /* load a json file or something */;
    json::Parser parser{ std::begin(json_text), std::end(json_text) };

    json::Value parsed_value = parser.parse([](const json::Error& error)
                                            {
                                                /* handle the error */
                                            });
    
    if (parsed_value.type() != json::e_JsonType::Null)
    {
        /* do something with parsed_value */
    }

    return 0;
}


Documentation

IMPORTANT! The Parser makes no copies of the input, including the values 
returned from the Parser. This means that the input to the Parser MUST outlive
the Parser and the values returned from it.


json_parser.hpp
===============
class Parser summary

Parser() = delete

Parser(const char* input)  
Construct using input. Does not copy input.

Parser(const char* start, const char* end)  
Construct start and end. This parse from the range [start, end). 
Does not copy from the range.

void reset()  
Reset the parser back to the beginning of the input.

void reset(const char* input)  
Reset the parser and change the input. Does not copy input.

void reset(const char* start, const char* end)  
Reset the parser and change the input to parse from the range [start, end).
Does not copy from the range.

Value parse(std::function<void(const Error&)> error_fun)  
This performs the parsing. Upon success it returns a Value. 
If there is an error of any kind, error_fun will be called and 
a Value of JSON null will be returned.


json_value.hpp
==============

The various types that a Value can hold.
enum class e_JsonType
{
    Object,
    Array,
    String,
    Integer,
    FloatingPoint,
    True,
    False,
    Null
}

typedef std::vector<Value> Array  
An array is a simple vector.

class Value summary

Value()  
Default constructs a Value with type e_JsonType::Null.

Value(const Object& obj)
Value(Object&& obj)  
Construct from an Object.

Value(const Array& arr)
Value(Array&& arr)  
Construct from an Array.

Value(const String& str)  
Construct from a String.

Value(long long i)  
Construct from a long long.

Value(double d)  
Construct from a double.

Value(bool b)
Construct from a bool.

Value(const Value&)
Value(Value&&)
Value& operator=(const Value&)
Value& operator=(Value&&)  
Values are CopyConstructible, CopyAssignable, MoveConstructible, and MoveAssignable.

e_JsonType type() const  
Get the e_JsonType representing the type the Value is holding.

template <e_JsonType J>  
unspecified& get()

template <e_JsonType J>  
const unspecified& get() const
Given an e_JsonType assume the Value holds that type and return it.  
Example:  
Value v{42};  
std::cout << v.get<e_JsonType::Integer>() << std::endl;  
This would print 42 to stdout. Substiting a value of e_JsonType that does not
match the type of the Value will most likely lead to undefined behavior of some kind.


json_object.hpp
===============

class Object summary

typedef std::pair<String, Value> Member

Value& operator[](const char* str)
const Value& operator[](const char* str) const
Value& operator[](const std::string& str)
const Value& operator[](const std::string& str) const
Get the Value associated with str. This does not work like std::map's operator[].
Calling operator[] with a string not in the object will cause undefined behavior.

iterator begin()
const_iterator begin() const
iterator end()
const_iterator end() const
Get an iterator over the pairs stored in the object.

bool empty() const

std::size_t size() const




json_string.hpp
===============

class String summary

This class is just a wrapper around two const char*'s.

