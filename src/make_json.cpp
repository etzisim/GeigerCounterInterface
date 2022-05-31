#include "make_json.h"
#include <Arduino.h>
#include <string.h>

String genJson::getJson()
{
    String json = "{\"" + json_data + "\"}";
    return json;
};

void genJson::add(String name, String value)
{
    if (json_data != "")
    {
        json_data = json_data + "\",\"";
    }
    json_data = json_data + name + "\":\"" + value;
};

void genJson::clear()
{
    json_data = "";
};

genJson::genJson()
{
    json_data = "";
};