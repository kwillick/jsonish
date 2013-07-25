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
    jsonish::Parser parser{json_text.c_str()};

    jsonish::Value parsed_value = parser.parse([](const jsonish::Error& error)
                                               {
                                                   /* handle the error */
                                               });
    
    if (parsed_value.type() != jsonish::e_JsonType::Null)
    {
        /* do something with parsed_value */
    }

    return 0;
}


Documentation

IMPORTANT! The Parser makes no copies of the input, including the values 
returned from the Parser. This means that the input to the Parser MUST have 
a longer lifetime than the Parser and the values returned from it.

Parser summary
====================

Parser class
------------

Parser() = delete

Parser(const char* input)  
Construct using input. Does not copy input.

Parser(const char* start, const char* end)  
Construct using start and end. This parses from the range [start, end). 
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
a Value with type e_JsonType::Null will be returned.


Value summary
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


String class
------------
Holds two pointers, one to the start of the string and one just past the end.


The String class has 3 important public member functions:
  std::string to_string() const
  Constructs a std::string from the two pointers and returns it.
  This makes a copy of the string data.

  const char* begin() const
  const char* end() const
  Get the pointers.


Value class
-----------
The Value class is DefaultConstructible, CopyConstructible, CopyAssignable,
MoveConstructible, and MoveAssignable.

The Value class has the follwing constructors:
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

The Value class has three important public member functions:
  e_JsonType type() const
  Get the e_JsonType representing the type the Value is holding.

  template <e_JsonType J>  
  unspecified& get()

  template <e_JsonType J>  
  const unspecified& get() const
  The unspecified return type is computed based on the e_JsonType template 
  parameter. This member function assumes the Value holds the type matching 
  the e_JsonType and unconditionally returns it. 
  Example:  
  Value v{42};
  std::cout << v.get<e_JsonType::Integer>() << std::endl;  
  This would print 42 to stdout. Substiting a value of e_JsonType that does not
  match the type of the Value will lead to undefined behavior.


Object class
------------
The Object class represents a JSON object using a
std::map<jsonish::String, jsonish::Value>.
The Object class is DefaultConstructible, CopyConstructible, CopyAssignable,
MoveConstructible, and MoveAssignable.

Object class public member functions:
  Value& operator[](const char* str)
  const Value& operator[](const char* str) const
  Value& operator[](const std::string& str)
  const Value& operator[](const std::string& str) const

  Get the Value associated with str. 
  This does not work like std::map's operator[].
  Calling operator[] with a string not in the object will cause undefined 
  behavior.


  iterator find(const char* key)
  iterator find(const std::string& key)
  const_iterator find(const char* key) const
  const_iterator find(const std::string& key) const

  Perform a search for key. Works the same as std::map's find.


  iterator begin()
  const_iterator begin() const
  iterator end()
  const_iterator end() const

  Get an iterator over the pairs stored in the object.

  bool empty() const

  std::size_t size() const


