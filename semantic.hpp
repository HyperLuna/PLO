#ifndef SEMANTIC_H
#define SEMANTIC_H

#include<iostream>
#include<memory>
#include<string>
#include<list>
#include<forward_list>
#include<initializer_list>
using std::unique_ptr;
using std::string;
using std::pair;
using std::list;
template<class T>
using flist=std::forward_list<T>;

#include "vm.hpp"
using namespace vm;

namespace semantic
{
class Procedure;
using Proc=Procedure;
class Statement{
public:
    virtual void translate(const Proc*)const=0;
    virtual ~Statement(){}
};
using Stat=Statement;

class Procedure
{
    mutable int code_pos=-1;
    mutable flist<I*>calls;

    int var_len=0;
    flist<string>vars;
    flist<pair<string,int>>defs;
    flist<unique_ptr<Proc>>procs;

    unique_ptr<Stat>stat;
protected:
    string name;
    const Proc*prev;
public:
    Procedure(string name,Stat*stat)
        :stat(stat),name(name){}
    virtual ~Procedure(){}
    void add_def(string name,int value)
    {
        defs.emplace_front(name,value);
    }
    void add_var(string name)
    {
        vars.push_front(name);
        ++var_len;
    }
    void add_proc(Proc*proc)
    {
        proc->prev=this;
        procs.emplace_front(proc);
    }
    virtual pair<int,int>find_val(string name,int lv=0)const
    {
        for(auto&x:defs)
            if(x.first==name)
                return std::make_pair(-1,x.second);

        auto it=vars.begin();
        for(int i=0;it!=vars.end();++it,++i)
            if(*it==name)
                return std::make_pair(lv,i);

        return prev->find_val(name,lv+1);
    }
    virtual pair<int,int>find_var(string name,int lv=0)const
    {
        auto it=vars.begin();
        for(int i=0;it!=vars.end();++it,++i)
            if(*it==name)
                return std::make_pair(lv,i);

        return prev->find_var(name,lv+1);
    }
    virtual void find_proc(string name,I*inst)const
    {
        for(auto&x:procs)
            if(x->name==name)
            {
                x->calls.push_front(inst);
                return;
            }
        return prev->find_proc(name,inst);
    }

    virtual void translate()const
    {
        code_pos=VM::code_pos();
        if(var_len)VM::write({I::INT,0,var_len});
        stat->translate(this);
        VM::write({I::RET,0,0});
       
        for(;!calls.empty();calls.pop_front())
            calls.front()->a=code_pos;

        for(auto it=procs.begin();it!=procs.end();++it)
            (*it)->translate();
    }
};
class MainProc:public Proc
{
    class VirtualProc:public Proc
    {
    public:
        VirtualProc():Proc("",nullptr){}
        pair<int,int>find_val(string,int)const
        {
            return std::make_pair(-1,0);
        }
        pair<int,int>find_var(string,int lv)const
        {
            return std::make_pair(lv-1,-2);
        }
        void find_proc(string,I*inst)const
        {
            inst->a=0;
        }
        void translate()const
        {
            VM::write({I::JMP,0,0});
        }
    }static vproc;
public:
    MainProc(Stat*stat):Proc("",stat)
    {
        prev=&vproc;
    }
    void translate()const
    {
        vproc.translate();
        Proc::translate();
    }
};

class Expression
{
public:
    virtual void translate(const Proc*)const=0;
    virtual void print(int)const=0;
    virtual bool check()const{return true;}
    virtual ~Expression(){}
};
using Expr=Expression;

class IntExpr:public Expr
{
    int num;
public:
    IntExpr(int num):num(num){}
    void translate(const Proc*)const
    {
        VM::write({I::LIT,0,num});
    }
    void print(int)const
    {
        std::cout<<"Int "<<num<<std::endl;
    }
};

class IdentExpr:public Expr
{
    string name;
public:
    IdentExpr(string name):name(name){}
    void translate(const Proc*proc)const
    {
        auto pair=proc->find_val(name);
        if(pair.first<0)
            VM::write({I::LIT,0,pair.second});
        else 
            VM::write({I::LOD,pair.first,pair.second});
    }
    void print(int)const
    {
        std::cout<<"Ident "<<name<<std::endl;
    }
};
class AddSubExpr:public Expr
{
    flist<unique_ptr<Expr>>adds;
    flist<unique_ptr<Expr>>subs;
public:
    void add(Expr*expr)
    {
        adds.push_front(unique_ptr<Expr>(expr));
    }
    void sub(Expr*expr)
    {
        subs.push_front(unique_ptr<Expr>(expr));
    }
    void translate(const Proc*proc)const
    {
        auto it=adds.begin();
        if(it!=adds.end())
        {
            (*it)->translate(proc);
            for(++it;it!=adds.end();++it)
            {
                (*it)->translate(proc);
                VM::write({I::OPR,0,I::ADD});
            }
            
        }
        else VM::write({I::LIT,0,0});
        for(auto&x:subs)
        {
            x->translate(proc);
            VM::write({I::OPR,0,I::SUB});
        }
    }
    void print(int lv)const
    {
        std::cout<<"AddSubExpr"<<std::endl;
        for(auto&x:adds)
        {
            for(int i=0;i<lv;++i)std::cout<<'\t';
            std::cout<<"+:";
            x->print(lv+1);
        }
        for(auto&x:subs)
        {
            for(int i=0;i<lv;++i)std::cout<<'\t';
            std::cout<<"-:";
            x->print(lv+1);
        }
    }
};
class MulDivExpr:public Expr
{
    flist<unique_ptr<Expr>>muls;
    flist<unique_ptr<Expr>>divs;
public:
    void mul(Expr*expr)
    {
        muls.push_front(unique_ptr<Expr>(expr));
    }
    void div(Expr*expr)
    {
        divs.push_front(unique_ptr<Expr>(expr));
    }
    void translate(const Proc*proc)const
    {
        auto it=muls.begin();
        (*it)->translate(proc);
        for(++it;it!=muls.end();++it)
        {
            (*it)->translate(proc);
            VM::write({I::OPR,0,I::MUL});
        }
        for(auto&x:divs)
        {
            x->translate(proc);
            VM::write({I::OPR,0,I::DIV});
        }
    }
    void print(int lv)const
    {
        std::cout<<"MulDivExpr"<<std::endl;
        for(auto&x:muls)
        {
            for(int i=0;i<lv;++i)std::cout<<'\t';
            std::cout<<"*:";
            x->print(lv+1);
        }
        for(auto&x:divs)
        {
            for(int i=0;i<lv;++i)std::cout<<'\t';
            std::cout<<"/:";
            x->print(lv+1);
        }
    }
};
class Condition
{
    unique_ptr<Expr>lhs,rhs;
    int opr;
public:
    Condition(Expr*lhs,Expr*rhs,int opr)
        :lhs(lhs),rhs(rhs),opr(opr){}
    void translate(const Proc*proc)const
    {
        lhs->translate(proc);
        rhs->translate(proc);
        VM::write({I::OPR,0,opr});
    }
};
using Cond=Condition;

class AssignStat:public Stat
{
    string id;
    unique_ptr<Expr>expr;
public:
    AssignStat(string ident,Expr*expr)
        :id(ident),expr(expr){}
    void translate(const Proc*proc)const
    {
        expr->translate(proc);
        auto pair=proc->find_var(id);
        VM::write({I::STO,pair.first,pair.second});
    }
};
class CompoundStat:public Stat
{
    list<unique_ptr<Stat>>stats;
public:
    void add_stat(Stat*stat)
    {
        stats.emplace_back(stat);
    }
    void translate(const Proc*proc)const
    {
        for(auto it=stats.begin();it!=stats.end();++it)
            (*it)->translate(proc);
    }

};
class IfStat:public Stat
{
    unique_ptr<Cond>cond;
    unique_ptr<Stat>stat;
public:
    IfStat(Cond*cond,Stat*stat)
        :cond(cond),stat(stat){}
    void translate(const Proc*proc)const
    {
        cond->translate(proc);
        I* inst=VM::write({I::JPC,0,0});
        stat->translate(proc);
        inst->a=VM::code_pos();
    }
};
class CallStat:public Stat
{
    string id;
public:
    CallStat(string ident):id(ident){}
    void translate(const Proc*proc)const
    {
        proc->find_proc(id,VM::write({I::CAL,0,0}));
    }
};
class WhileStat:public Stat
{
    unique_ptr<Cond>cond;
    unique_ptr<Stat>stat;
public:
    WhileStat(Cond*cond,Stat*stat)
        :cond(cond),stat(stat){}
    void translate(const Proc*proc)const
    {
        int pos=VM::code_pos();
        cond->translate(proc);
        I* inst=VM::write({I::JPC,0,0});
        stat->translate(proc);
        VM::write({I::JMP,0,pos});
        inst->a=VM::code_pos();
    }
};
}

#endif
