#ifndef READER_H
#define READER_H

#include<experimental/any>
#include<unordered_map>
#include<fstream>
#include<string>
#include<cctype>

using std::string;
using std::ifstream;
using std::experimental::any;
using std::experimental::any_cast;
template<class K,class T>
using umap=std::unordered_map<K,T>;
using pos_t=ifstream::pos_type;

class Reader
{
    ifstream ifile;
    char last=0;
    //umap<pos_t,any>cache;
public:
    Reader(const string&path):ifile(path){}
    pos_t tell()
    {
        return ifile.tellg();
    }
    void seek(pos_t pos)
    {
        ifile.seekg(pos);
    }
    void back()
    {
        if(last)ifile.unget();
    }
    char peek()
    {
        if(ifile.eof())return 0;
        char ch=ifile.peek();
        if(isspace(ch))return 0;
        else return ch;
    }
    char read()
    {
        if(ifile.eof())
        {
            last=0;
            return 0;
        }
        char ch=ifile.get();
        if(isspace(ch))ch=0;
        last=ch;
        return ch;
    }
    void skip()
    {
        while(!ifile.eof()&&isspace(ifile.peek()))
            ifile.get();
    }
};

#endif
