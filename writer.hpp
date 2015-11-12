#ifndef WRITER_H
#define WRITER_H

#include<fstream>
#include<string>

using std::string;
using std::ofstream;

class Writer
{
    ofstream ofile;
    int indent=0;
public:
    Writer(const string&path):ofile(path){}
    ~Writer()
    {
        ofile.close();
    }
    void inc()
    {
        ++indent;
    }
    void dec()
    {
        --indent;
    }
    void write(const string&str)
    {
        ofile.put('\n');
        for(int i=0;i<indent;++i)
            ofile.put('\t');
        ofile<<str;
    }
};

#endif
