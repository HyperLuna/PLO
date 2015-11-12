#ifndef VM_H
#define VM_H

#include<iostream>

namespace vm
{
struct Instruction
{
    enum FUN_ID
    {
        NOP=0,
        LIT,LOD,STO,
        CAL,RET,INT,
        JMP,JPC,
        OPR
    };
    enum OPR_ID
    {
        EXIT=-1,
        NEG,
        ADD,SUB,
        MUL,DIV,

        EQ,NE,
        GT,GE,
        LT,LE,
    };
    int f,l,a;
};
typedef Instruction I;

class VM
{
    static constexpr unsigned short CODE_MAX=1000;
    static constexpr unsigned short STACK_MAX=1000;

    static Instruction code[CODE_MAX];
    static int code_len;

    static int stack[STACK_MAX];
    static int ip,sp,bp;

    static int trace(int lv)
    {
        int b=bp;
        for(;lv>0;--lv)b=stack[b];
        return b;
    }
    static int trace(int lv,int pos)
    {
        return trace(lv)+3+pos;
    }

    static void nop(int,int){}
    static void lit(int,int a){stack[++sp]=a;}
    static void lod(int l,int a){stack[++sp]=stack[trace(l,a)];}
    static void sto(int l,int a){stack[trace(l,a)]=stack[sp--];}
    static void cal(int l,int a)
    {
        stack[sp+1]=trace(l);
        stack[sp+2]=bp;
        stack[sp+3]=ip;

        bp=sp+1;
        sp+=3;
        ip=a-1;
    }
    static void ret(int,int)
    {
        ip=stack[bp+2];
        sp=bp-1;
        bp=stack[bp+1];
    }
    static void inT(int,int a){sp+=a;}
    static void jmp(int,int a){ip=a-1;}
    static void jpc(int,int a){if(!stack[sp])ip=a-1;}

    static void opr_neg(){++sp;stack[sp]=-stack[sp];}
    static void opr_add(){stack[sp]+=stack[sp+1];}
    static void opr_sub(){stack[sp]-=stack[sp+1];}
    static void opr_mul(){stack[sp]*=stack[sp+1];}
    static void opr_div(){stack[sp]/=stack[sp+1];}
    static void opr_eq(){stack[sp]=(stack[sp]==stack[sp+1]);}
    static void opr_ne(){stack[sp]=(stack[sp]!=stack[sp+1]);}
    static void opr_gt(){stack[sp]=(stack[sp]>stack[sp+1]);}
    static void opr_ge(){stack[sp]=(stack[sp]>=stack[sp+1]);}
    static void opr_lt(){stack[sp]=(stack[sp]<stack[sp+1]);}
    static void opr_le(){stack[sp]=(stack[sp]<=stack[sp+1]);}

    constexpr static void(*opr_fn[])()
    {
        opr_neg,
        opr_add,opr_sub,
        opr_mul,opr_div,
        opr_eq,opr_ne,
        opr_gt,opr_ge,
        opr_lt,opr_le
    };
    static void opr(int,int a){--sp;opr_fn[a]();}

    constexpr static void(*fun[])(int,int)
    {
        nop,
        lit,lod,sto,
        cal,ret,inT,
        jmp,jpc,
        opr
    };

public:
    static Instruction* write(Instruction inst)
    {
        code[code_len]=inst;
        return code+(code_len++);
    }
    static int code_pos(){return code_len;} 

    static const char*fun_names[];
    static const char*opr_names[];

    static void print()
    {
        for(int i=0;i<code_len;++i)
        {
            std::cout<<i<<'\t';
            std::cout<<fun_names[code[i].f]<<' '<<code[i].l<<' ';
            if(code[i].f==I::OPR)std::cout<<opr_names[code[i].a];
            else std::cout<<code[i].a;
            std::cout<<std::endl;
        }
    }

    static void reset()
    {
        code_len=0;
    }
    static void init()
    {
        ip=1;
        bp=0;
        stack[2]=0;
        sp=2;
    }
    static void step()
    {
        fun[code[ip].f](code[ip].l,code[ip].a);
        ++ip;
    }
};
}

#endif
