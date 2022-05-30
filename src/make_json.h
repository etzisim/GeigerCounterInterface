#ifndef make_json_h
#define make_json_h

#include <Arduino.h>
#include <string.h>

String start_json(String name, String value);
String add_to_json(String input, String name, String value);
String end_json(String input);

#endif