#include "avg_calc.h"
#include <list>

void avg_calc::addVal(float new_val)
{
    _Buffer.push_front(new_val);

    while (_Buffer.size() > BufferSize)
    {
        _Buffer.pop_back();
    }
};

void avg_calc::SetBufferSize(int n)
{
    BufferSize = n;
};

int avg_calc::GetSize()
{
    return _Buffer.size();
};

float avg_calc::getAverage()
{
    float average = 0;
    for (float x : _Buffer)
    {
        average = average + x;
    }
    average = average / _Buffer.size();

    return average;
};

avg_calc::avg_calc(int size){
    avg_calc::SetBufferSize(size);
};
