#include "make_json.h"
#include <Arduino.h>
#include <string.h>

String start_json(String name, String value){
    String json = "{\"" + name + "\":\"" + value;
    return json;
};
String add_to_json(String input, String name, String value){
    String json = input + "\",\"" + name + "\":\"" + value;
    return json;
};

String end_json(String input){
    String json = input + "\"}";
    return json;
};