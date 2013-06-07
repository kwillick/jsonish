#include <iostream>
#include <string>
#include "../json_writer.hpp"

int main(int argc, char *argv[])
{
    using json::Object; 
    using json::Array;
    using json::Value;

    Object object{
        { "empty", Object{
                { "object", Object{} },
                { "array", Array{} }
            }
        },
        { "nonempty", Object{
                { "array", Array{"string", 1, 1.2, true, false, Value{} } }
            }
        }
    };

    json::write(std::cout, object);
    
    std::cout << std::endl;

    json::write_pretty(std::cout, object);
    std::cout << std::endl;
    
    return 0;
}
