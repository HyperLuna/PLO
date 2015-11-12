#ifndef SYNTAX_H
#define SYNTAX_H

#include<initializer_list>
#include<forward_list>
#include<functional>
#include<vector>
#include<string>
#include<cctype>
using std::pair;
using std::vector;
using std::string;
using std::function;
template<class T>
using ilist=std::initializer_list<T>;
template<class T>
using flist=std::forward_list<T>;

#include<experimental/any>
using std::experimental::any;
using std::experimental::any_cast;

#include "semantic.hpp"
using namespace semantic;
#include "reader.hpp"

namespace syntax
{
class Expression
{
public:
    virtual any match(Reader&)const=0;
    virtual ~Expression(){};
};
using E=Expression;

class Token:public E
{
public:
    any match(Reader&)const
    {
        return (E*)this;
    }
};

class That:public E
{
    function<any(Reader&)>fn;
public:
    any match(Reader&rd)const
    {
        return fn(rd);
    }
    That(function<any(Reader&)>fn):fn(fn){}
};
class Keyword:public E
{
    string kw;
    bool ns;
public:
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        for(auto it=kw.begin();it!=kw.end();++it)
        {
            if(*it!=rd.read())
            {
                rd.seek(pos);
                return any();
            }
        }
        if(ns&&rd.read()!=0)
        {
            rd.seek(pos);
            return any();
        }
        else return (E*)this;
    }
    Keyword(string keyword,bool need_space=false):kw(keyword),ns(need_space){}
};

class First:public E
{
    flist<E*>list;
public:
    First(ilist<E*>list):list(list){}
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        for(auto&x:list)
        {
            any r=x->match(rd);
            if(!r.empty())return r;
            rd.seek(pos);
        }
        return any();
    }
};

class Optional:public E
{
    E* expr;
public:
    Optional(E*expr):expr(expr){}
    any match(Reader&rd)const
    {
        extern Token empty;
        any r=expr->match(rd);
        if(!r.empty())return r;
        else return (E*)&empty;
    }
};
class Sequence:public E
{
    flist<E*>list;
    function<any(const vector<any>&)>fn;
public:
    Sequence(ilist<E*>list,function<any(const vector<any>&)>fn)
        :list(list),fn(fn){}
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        vector<any>vec;
        for(auto it=list.begin();it!=list.end();++it)
        {
            any r=(*it)->match(rd);
            if(r.empty())
            {
                rd.seek(pos);
                break;
            }
            vec.push_back(r);
        }
        return fn(vec);
    }
};
class Repeat:public E
{
    int max;
    function<any(const vector<any>&)>fn;
protected:
    E* expr;
public:
    Repeat(E*expr,
        function<any(const vector<any>&)>fn=
        [](const vector<any>&vec)->any
        {
            return vec;
        },int max=1000000)
        :max(max),fn(fn),expr(expr){}
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        vector<any>vec;
        for(int i=0;i<max;++i)
        {
            any r=expr->match(rd);
            if(r.empty())break;
            vec.push_back(r);
        }
        any t=fn(vec);
        if(t.empty())rd.seek(pos);
        return t;
    }
};
class RepeatSequence:public Repeat
{
public:
    RepeatSequence(ilist<E*>list,function<any(const vector<any>&)>seq_fn,
        function<any(const vector<any>&)>fn=
        [](const vector<any>&vec)->any
        {
            return vec;
        },int max=1000000)
        :Repeat(new Sequence(list,seq_fn),fn,max){}
    ~RepeatSequence()
    {
        delete expr;
    }
};

extern Sequence main_proc;
}

#endif
