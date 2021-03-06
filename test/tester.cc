#include <fstream>
#include <string>
#include <iostream>
#include <utility>
#include "../jsonish.hpp"

std::string red(const std::string& s);
std::string blue(const std::string& s);

std::string read_file(const std::string& filename);

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

    jsonish::Parser parser{text};
    bool parse_error = false;
    jsonish::Error error;
    jsonish::Value result = parser.parse([&parse_error,&error](const jsonish::Error& err)
                                         {
                                             error = err;
                                             parse_error = true;
                                         });

    if (expect_pass && !parse_error)
    {
        if (toplevel_object)
        {
            if (result.type() != jsonish::e_JsonType::Object)
            {
                std::cout << "test " << red("FAILED") << " expected top level object\n\n";
                return 1;
            }

            std::cout << "test " << blue("PASSED") << ", result:\n";
            jsonish::write_pretty(std::cout, result.get<jsonish::e_JsonType::Object>());
            std::cout << '\n';
        }
        else
        {
            if (result.type() != jsonish::e_JsonType::Array)
            {
                std::cout << "test " << red("FAILED") << " expected top level array\n\n";
                return 1;
            }

            std::cout << "test " << blue("PASSED") << ", result:\n";
            jsonish::write_pretty(std::cout, result.get<jsonish::e_JsonType::Array>());
            std::cout << '\n';
        }
    }
    else if (expect_pass && parse_error)
    {
        //unexpected error
        std::cout << "test " << red("FAILED") << ": '" << error.message << "'\n\n";
        return 1;
    }
    else if (!expect_pass && !parse_error)
    {
        //error was expected but didn't happen
        std::cout << "test " << red("FAILED") << ": expected parse error\n\n";
        return 1;
    }
    else if (!expect_pass && parse_error)
    {
        //error expected and it happened
        std::cout << "test " << blue("PASSED") << ", expected parse error. Error is: '" 
                  << error.message << "'\n";
    }

    std::cout << std::endl;
    
    return 0;
}

std::string red(const std::string& s)
{
    return "\033[31m" + s + "\033[0m";
}

std::string blue(const std::string& s)
{
    return "\033[34m" + s + "\033[0m";
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
