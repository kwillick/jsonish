#include <fstream>
#include <string>
#include "../json_writer.hpp"

int main(int argc, char *argv[])
{
    std::ofstream out(argv[1]);

    using json::writer::Object; 
    using json::writer::Array;
    using json::writer::Value;

    json::writer::write(out, 
                        Object{
                            { "empty", Object{
                                    { "object", Object{} },
                                    { "array", Array{} }
                                }
                            },
                            { "nonempty", Object{
                                    { "array", Array{"string", 1, 1.2, true, false, Value{} } }
                                }
                            }
                        });
    
    return 0;
}
