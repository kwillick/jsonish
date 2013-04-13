#include "json_debug.hpp"
#include <iostream>

namespace json
{

void print_value(const json::Value& v)
{
    switch(v.type())
    {
    case json::e_JsonType::Object:
        print_object(v.get<json::e_JsonType::Object>());
        break;
    case json::e_JsonType::Array:
        print_array(v.get<json::e_JsonType::Array>());
        break;
    case json::e_JsonType::String:
        std::cout << '"' << v.get<json::e_JsonType::String>().to_string() << '"';
        break;
    case json::e_JsonType::Integer:
        std::cout << v.get<json::e_JsonType::Integer>();
        break;
    case json::e_JsonType::FloatingPoint:
        std::cout << v.get<json::e_JsonType::FloatingPoint>();
        break;
    case json::e_JsonType::True:
        std::cout << "true";
        break;
    case json::e_JsonType::False:
        std::cout << "false";
        break;
    case json::e_JsonType::Null:
        std::cout << "null";
        break;
    }
}

static const char* s_indent = "    ";
static inline void _print_indent(int n)
{
    for (int i = 0; i < n; i++)
        std::cout << s_indent;
}

void print_object(const json::Object& object, int indent)
{
    std::cout << "{\n";
    auto last = object.end() - 1;
    for (auto it = object.begin(); it != object.end(); ++it)
    {
        _print_indent(indent);
        const auto& pair = *it;
        std::cout << '"' << pair.first.to_string() << '"' << ": ";
        const json::Value& v = pair.second;
        switch (v.type())
        {
        case json::e_JsonType::Object:
            print_object(v.get<json::e_JsonType::Object>(), indent + 1);
            break;
        case json::e_JsonType::Array:
            print_array(v.get<json::e_JsonType::Array>(), indent + 1);
            break;
        case json::e_JsonType::String:
            std::cout << '"' << v.get<json::e_JsonType::String>().to_string() << '"';
            break;
        case json::e_JsonType::Integer:
            std::cout << v.get<json::e_JsonType::Integer>();
            break;
        case json::e_JsonType::FloatingPoint:
            std::cout << v.get<json::e_JsonType::FloatingPoint>();
            break;
        case json::e_JsonType::True:
            std::cout << "true";
            break;
        case json::e_JsonType::False:
            std::cout << "false";
            break;
        case json::e_JsonType::Null:
            std::cout << "null";
            break;
        }

        if (it != last)
            std::cout << ",\n";
        else
            std::cout << '\n';
    }

    _print_indent(indent - 1);
    std::cout << '}';
    if (indent == 1)
        std::cout << std::endl;
}

void print_array(const json::Array& array, int indent)
{
    std::cout << "[\n";
    auto last = array.end() - 1;
    for (auto it = array.begin(); it != array.end(); ++it)
    {
        _print_indent(indent);
        const json::Value& v = *it;
        switch (v.type())
        {
        case json::e_JsonType::Object:
            print_object(v.get<json::e_JsonType::Object>(), indent + 1);
            break;
        case json::e_JsonType::Array:
            print_array(v.get<json::e_JsonType::Array>(), indent + 1);
            break;
        case json::e_JsonType::String:
            std::cout <<'"' << v.get<json::e_JsonType::String>().to_string() << '"';
            break;
        case json::e_JsonType::Integer:
            std::cout << v.get<json::e_JsonType::Integer>();
            break;
        case json::e_JsonType::FloatingPoint:
            std::cout << v.get<json::e_JsonType::FloatingPoint>();
            break;
        case json::e_JsonType::True:
            std::cout << "true";
            break;
        case json::e_JsonType::False:
            std::cout << "false";
            break;
        case json::e_JsonType::Null:
            std::cout << "null";
            break;
        }

        if (it != last)
            std::cout << ",\n";
        else
            std::cout << '\n';
    }

    _print_indent(indent - 1);
    std::cout << ']';
    if (indent == 1)
        std::cout << std::endl;
}

} //json
