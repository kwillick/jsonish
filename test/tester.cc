#include <fstream>
#include <string>
#include <iostream>
#include <utility>
#include <deque>
#include "../json.hpp"

std::string read_file(const std::string& filename);

void print_object(const json::Object& object, int indent = 1);
void print_array(const json::Array& array, int indent = 1);

int main(int argc, char *argv[])
{
    std::string filename(argv[1]);
    std::string text = read_file(filename);

    std::string expect(argv[2]);
    bool expect_pass = expect == "pass";

    std::string top;
    bool toplevel_object = false;
    if (expect_pass)
    {
        top = argv[3];
        toplevel_object = top == "object";
    }
    
    std::cout << "test: " << filename << " expected " << (expect_pass ? "pass" : "fail") << "\n";

    const char* start = &*text.begin();
    const char* end = start + text.length();
    json::Parser parser(start, end);
    bool parse_error = false;
    json::Value result = parser.parse([&parse_error](const json::Error& error)
                                      {
                                          parse_error = true;
                                      });

    if (expect_pass && !parse_error)
    {
        if (toplevel_object)
        {
            if (result.type() != json::e_JsonType::Object)
            {
                std::cout << "test FAILED expected top level object\n\n";
                return 1;
            }

            std::cout << "test PASSED, result:\n";
            print_object(result.get<json::e_JsonType::Object>());
        }
        else
        {
            if (result.type() != json::e_JsonType::Array)
            {
                std::cout << "test FAILED expected top level array\n\n";
                return 1;
            }

            std::cout << "test PASSED, result:\n";
            print_array(result.get<json::e_JsonType::Array>());
        }
    }
    else if (expect_pass && parse_error)
    {
        //unexpected error
        //TODO add actual errors in parser
        std::cout << "test FAILED: \n\n";
        return 1;
    }
    else if (!expect_pass && !parse_error)
    {
        //error was expected but didn't happen
        std::cout << "test FAILED: expected parse error\n\n";
        return 1;
    }
    else if (!expect_pass && parse_error)
    {
        //error expected and it happened
        std::cout << "test PASSED, expected parse error. Error is: \n";
    }

    std::cout << std::endl;
    
    return 0;
}

std::string read_file(const std::string& filename)
{
    std::ifstream stream(filename, std::ios::in | std::ios::ate);
    
    std::string result;
    result.reserve(stream.tellg());

    stream.seekg(0, std::ios::beg);
    result.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

    return result;
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
