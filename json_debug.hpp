#ifndef JSON_DEBUG_H
#define JSON_DEBUG_H

#include "json.hpp"

namespace json
{

void print_value(const json::Value& v);
void print_object(const json::Object& obj, int indent = 1);
void print_array(const json::Array& arr, int indent = 1);

} //json

#endif //JSON_DEBUG_H
