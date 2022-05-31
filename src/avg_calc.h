#ifndef calc_avg_h
#define calc_avg_h

#include <list>

class avg_calc
{
public:
    float getAverage();
    void addVal(float new_val);
    void SetBufferSize(int n);
    int GetSize();
    avg_calc(int size);

private:
    unsigned int BufferSize;
    std::list<float> _Buffer;
};
#endif