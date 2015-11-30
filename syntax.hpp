#ifndef SYNTAX_H
#define SYNTAX_H

#include "writer.hpp"
extern Writer log_syn;

#include<initializer_list>
#include<forward_list>
#include<functional>
#include<vector>
#include<string>
#include<cctype>
using std::pair;
using std::vector;
using std::string;
using std::to_string;
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
class Parser
{
protected:
    //umap<pos_t,any>cache;
public:
    virtual any match(Reader&)const=0;
    virtual ~Parser(){};
};

class Token:public Parser
{
public:
    any match(Reader&)const
    {
        log_syn.write("Token");
        return (Parser*)this;
    }
};

class That:public Parser
{
    function<any(Reader&)>fn;
public:
    any match(Reader&rd)const
    {
        return fn(rd);
    }
    That(function<any(Reader&)>fn):fn(fn){}
};
class Keyword:public Parser
{
    string kw;
    bool ns;
public:
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        string out="Keyword:"+kw+" ";
        for(auto it=kw.begin();it!=kw.end();++it)
        {
            if(*it!=rd.read())
            {
                rd.seek(pos);
                out+="Failed";
                log_syn.write(out);
                return any();
            }
        }
        if(ns&&rd.read()!=0)
        {
            rd.seek(pos);
            out+="Failed";
            log_syn.write(out);
            return any();
        }
        else
        {
            out+="Succeed";
            log_syn.write(out);
            return (Parser*)this;
        }
    }
    Keyword(string keyword,bool need_space=false):kw(keyword),ns(need_space){}
};

class First:public Parser
{
    flist<Parser*>list;
public:
    First(ilist<Parser*>list):list(list){}
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        log_syn.write("First");
        log_syn.inc();
        for(auto&x:list)
        {
            any r=x->match(rd);
            if(!r.empty())
            {
                log_syn.dec();
                log_syn.write("First Succeed");
                return r;
            }
            rd.seek(pos);
        }
        log_syn.dec();
        log_syn.write("First Failed");
        return any();
    }
};

class Optional:public Parser
{
    Parser* expr;
public:
    Optional(Parser*expr):expr(expr){}
    any match(Reader&rd)const
    {
        extern Token empty;
        log_syn.write("Optional");
        log_syn.inc();
        any r=expr->match(rd);
        if(!r.empty())
        {
            log_syn.dec();
            log_syn.write("Optional Over");
            return r;
        }
        else
        {
            log_syn.dec();
            log_syn.write("Optional Over");
            return (Parser*)&empty;
        }
    }
};
class Sequence:public Parser
{
    flist<Parser*>list;
    function<any(const vector<any>&)>fn;
public:
    Sequence(ilist<Parser*>list,function<any(const vector<any>&)>fn)
        :list(list),fn(fn){}
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        log_syn.write("Sequence");
        log_syn.inc();
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
        log_syn.dec();
        return fn(vec);
    }
};
class Repeat:public Parser
{
    int max;
    function<any(const vector<any>&)>fn;
protected:
    Parser* expr;
public:
    Repeat(Parser*expr,
        function<any(const vector<any>&)>fn=
        [](const vector<any>&vec)->any
        {
            log_syn.write("Repeat:"+to_string(vec.size()));
            return vec;
        },int max=1000000)
        :max(max),fn(fn),expr(expr){}
    any match(Reader&rd)const
    {
        rd.skip();
        auto pos=rd.tell();
        log_syn.write("Repeat");
        log_syn.inc();
        vector<any>vec;
        for(int i=0;i<max;++i)
        {
            any r=expr->match(rd);
            if(r.empty())break;
            vec.push_back(r);
        }
        log_syn.dec();
        any t=fn(vec);
        if(t.empty())rd.seek(pos);
        return t;
    }
};
class RepeatSequence:public Repeat
{
public:
    RepeatSequence(ilist<Parser*>list,function<any(const vector<any>&)>seq_fn,
        function<any(const vector<any>&)>fn=
        [](const vector<any>&vec)->any
        {
            log_syn.write("Repeat:"+to_string(vec.size()));
            return vec;
        },int max=1000000)
        :Repeat(new Sequence(list,seq_fn),fn,max){}
    ~RepeatSequence()
    {
        delete expr;
    }
};
}

#endif
