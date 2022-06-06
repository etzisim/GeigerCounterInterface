#ifndef make_json_h
#define make_json_h

#include <Arduino.h>
#include <string.h>

class genJson
{
public:
    String getJson();
    void add(String name, String value);
    void clear();
    genJson();

private:
    String json_data;
};

#endif