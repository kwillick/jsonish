#include <iostream>
#include <fstream>
#include <string>
#include "../json.hpp"

std::string read_file(const std::string& filename);

int main(int argc, char *argv[])
{
    std::string filename{argv[1]};
    std::string text = read_file(filename);

    std::string output_filename{argv[2]};
    output_filename += ".json";

    std::string pretty_filename{argv[2]};
    pretty_filename += "_pretty.json";
    
    std::ofstream output{output_filename};
    std::ofstream output_pretty{pretty_filename};

    std::cout << "parsing " << filename << '\n';
    
    json::Parser parser{text};
    bool parse_error;
    json::Error error;
    json::Value result = parser.parse([&parse_error,&error](const json::Error& err)
                                      {
                                          error = err;
                                          parse_error = true;
                                      });
    if (parse_error)
    {
        std::cout << "unexpected parse error: '" << error.message << "'\n";
        return 1;
    }

    std::cout << "performing standard write to: " << output_filename << '\n';
    json::write(output, result);

    std::cout <<"performing pretty write to: " << pretty_filename << '\n';
    json::write_pretty(output_pretty, result);
    
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
