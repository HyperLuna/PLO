#include "syntax.hpp"

namespace syntax
{
Token empty;
That
integer(
        [](Reader&rd)->any
        {
            rd.skip();
            string out="Integer:";
            char ch=rd.read();
            if(!isdigit(ch))
            {
                rd.back();
                out+="Failed";
                log_syn.write(out);
                return any();
            }
            int num=0;
            do
            {
                num*=10;
                num+=ch-'0';
                ch=rd.read();
            }while(isdigit(ch));
            rd.back();
            out+=to_string(num);
            out+=" Succeed";
            log_syn.write(out);
            return num;
        }),
ident(
        [](Reader&rd)->any
        {
            rd.skip();
            string out="Identifier:";
            char ch=rd.read();
            if(!isalpha(ch))
            {
                rd.back();
                out+="Failed";
                log_syn.write(out);
                return any();
            }
            string id;
            do
            {
                id.push_back(ch);
                ch=rd.read();
            }while(isalpha(ch)||isdigit(ch));
            rd.back();
            out+=id;
            out+=" Succeed";
            log_syn.write(out);
            return id;
        });

Keyword
kw_const("const",true),kw_var("var",true),kw_proc("proc",true),
kw_begin("begin",true),kw_end("end",true),
kw_if("if",true),kw_then("then",true),
kw_while("while",true),kw_do("do",true),
kw_call("call",true),
kw_assign(":="),
kw_add("+"),kw_sub("-"),kw_mul("*"),kw_div("/"),
kw_eq("=="),kw_ne("!="),kw_ge(">="),kw_gt(">"),kw_le("<="),kw_lt("<"),
kw_lbr("("),kw_rbr(")"),
kw_comma(","),kw_colon(":"),kw_semicolon(";");

First opr_mul_div({&kw_mul,&kw_div}),opr_add_sub({&kw_add,&kw_sub});
Optional opt_add_sub(&opr_add_sub);

extern First expr;
Sequence expr_bracket({&kw_lbr,&expr,&kw_rbr},
        [](const vector<any>&vec)->any
        {
            if(vec.size()==2)
            {
                delete any_cast<Expr*>(vec[1]);
                log_syn.write("Sequence:expr_bracket Failed");
                return any();
            }
            else if(vec.size()!=3)
            {
                log_syn.write("Sequence:expr_bracket Failed");
                return any();
            }

            log_syn.write("Sequence:expr_bracket Succeed");
            return vec[1];
        });
That 
expr_integer(
        [](Reader&rd)->any
        {
            any r=integer.match(rd);
            if(r.empty())
            {
                log_syn.write("That:expr_integer Failed");
                return any();
            }
            log_syn.write("That:expr_integer Succeed");
            return (Expr*)new IntExpr(any_cast<int>(r));
        }),
expr_ident(
        [](Reader&rd)->any
        {
            any r=ident.match(rd);
            if(r.empty())
            {
                log_syn.write("That:expr_identifier Failed");
                return r;
            }
            log_syn.write("That:expr_identifier Succeed");
            return (Expr*)new IdentExpr(any_cast<string>(r));
        });

First expr_factor({&expr_integer,&expr_ident,&expr_bracket});

RepeatSequence expr_rep_mul_div_factor({&opr_mul_div,&expr_factor},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=2)
            {
                log_syn.write("Sequence:expr_mul_div_factor Failed");
                return any();
            }

            log_syn.write("Sequence:expr_mul_div_factor Succeed");
            return std::make_pair(
                    any_cast<Parser*>(vec[0]),
                    any_cast<Expr*>(vec[1]));
        },
        [](const vector<any>&vec)->any
        {
            if(!vec.size())
            {
                log_syn.write("Repeat:0 Failed");
                return any();
            }
            log_syn.write("Repeat:"+to_string(vec.size())+" Succeed");
            return vec;
        });
Sequence expr_mul_div({&expr_factor,&expr_rep_mul_div_factor},
        [](const vector<any>&vec)->any
        {
            if(vec.size()>0&&vec.size()<2)
            {
                delete any_cast<Expr*>(vec[0]);
                log_syn.write("Sequence:expr_mul_div Failed");
                return any();
            }
            else if(vec.size()!=2)
            {
                log_syn.write("Sequence:expr_mul_div Failed");
                return any();
            }

            auto ptr=new MulDivExpr();
            ptr->mul(any_cast<Expr*>(vec[0]));
            auto v=any_cast<vector<any>>(vec[1]);
            for(auto it=v.begin();it!=v.end();++it)
            {
                auto pr=any_cast<pair<Parser*,Expr*>>(*it);
                if(pr.first==&kw_mul)ptr->mul(pr.second);
                else ptr->div(pr.second);
            }
            log_syn.write("Sequence:expr_mul_div Succeed");
            return (Expr*)ptr;
        });
First expr_item({&expr_mul_div,&expr_factor});
RepeatSequence expr_rep_add_sub_item({&opr_add_sub,&expr_item},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=2)
            {
                log_syn.write("Sequence:expr_add_sub_item Failed");
                return any();
            }

            log_syn.write("Sequence:expr_add_sub_item Succeed");
            return std::make_pair(
                    any_cast<Parser*>(vec[0]),
                    any_cast<Expr*>(vec[1]));
        });
Sequence expr_add_sub({&opt_add_sub,&expr_item,&expr_rep_add_sub_item},
        [](const vector<any>&vec)->any
        {
            if(vec.size()==2)
            {
                delete any_cast<Expr*>(vec[1]);
                log_syn.write("Sequence:expr_add_sub_item Failed");
                return any();
            }
            else if(vec.size()!=3)
            {
                log_syn.write("Sequence:expr_add_sub_item Failed");
                return any();
            }

            auto ptr=new AddSubExpr();
            Parser* p=any_cast<Parser*>(vec[0]);
            auto v=any_cast<vector<any>>(vec[2]);
            if(p==&kw_sub)
                ptr->sub(any_cast<Expr*>(vec[1]));
            else if(p==&kw_add)
                ptr->add(any_cast<Expr*>(vec[1]));
            else
            {
                if(!v.size())
                {
                    delete any_cast<Expr*>(vec[1]);
                    delete ptr;
                    log_syn.write("Sequence:expr_add_sub_item Failed");
                    return any();
                }
                ptr->add(any_cast<Expr*>(vec[1]));
            }
            for(auto it=v.begin();it!=v.end();++it)
            {
                auto pr=any_cast<pair<Parser*,Expr*>>(*it);
                if(pr.first==&kw_add)ptr->add(pr.second);
                else ptr->sub(pr.second);
            }
            log_syn.write("Sequence:expr_add_sub_item Succeed");
            return (Expr*)ptr;
        });
First expr({&expr_add_sub,&expr_mul_div,&expr_factor});

extern Sequence if_stat,while_stat,call_stat,comp_stat,assign_stat;
First stat({&if_stat,&while_stat,&call_stat,&comp_stat,&assign_stat});
Repeat rep_stat(&stat);

First opr_cond({&kw_eq,&kw_ne,&kw_gt,&kw_ge,&kw_lt,&kw_le});

Sequence cond({&expr,&opr_cond,&expr},
        [](const vector<any>&vec)->any
        {
            if(vec.size()>0&&vec.size()<3)
            {
                delete any_cast<Expr*>(vec[0]);
                log_syn.write("Sequence:cond Failed");
                return any();
            }
            else if(vec.size()!=3)
            {
                log_syn.write("Sequence:cond Failed");
                return any();
            }

            auto p=any_cast<Parser*>(vec[1]);
            log_syn.write("Sequence:cond Succeed");
            if(&kw_eq==p)return new Cond(
                    any_cast<Expr*>(vec[0]),any_cast<Expr*>(vec[2]),I::EQ);
            if(&kw_ne==p)return new Cond(
                    any_cast<Expr*>(vec[0]),any_cast<Expr*>(vec[2]),I::NE);
            if(&kw_gt==p)return new Cond(
                    any_cast<Expr*>(vec[0]),any_cast<Expr*>(vec[2]),I::GT);
            if(&kw_ge==p)return new Cond(
                    any_cast<Expr*>(vec[0]),any_cast<Expr*>(vec[2]),I::GE);
            if(&kw_lt==p)return new Cond(
                    any_cast<Expr*>(vec[0]),any_cast<Expr*>(vec[2]),I::LT);
            if(&kw_le==p)return new Cond(
                    any_cast<Expr*>(vec[0]),any_cast<Expr*>(vec[2]),I::LE);
            return new Cond(
                    any_cast<Expr*>(vec[0]),any_cast<Expr*>(vec[2]),I::EXIT);
        });

Sequence
assign_stat({&ident,&kw_assign,&expr,&kw_semicolon},
        [](const vector<any>&vec)->any
        {
            if(vec.size()==3)
            {
                delete any_cast<Expr*>(vec[2]);
                log_syn.write("Sequence:assign_stat Failed");
                return any();
            }
            else if(vec.size()!=4)
            {
                log_syn.write("Sequence:assign_stat Failed");
                return any();
            }

            log_syn.write("Sequence:assign_stat Succeed");
            return (Stat*)new AssignStat(
                    any_cast<string>(vec[0]),any_cast<Expr*>(vec[2]));
        }),
if_stat({&kw_if,&cond,&kw_then,&stat},
        [](const vector<any>&vec)->any
        {
            if(vec.size()>1&&vec.size()<4)
            {
                delete any_cast<Cond*>(vec[1]);
                log_syn.write("Sequence:if_stat Failed");
                return any();
            }
            else if(vec.size()!=4)
            {
                log_syn.write("Sequence:if_stat Failed");
                return any();
            }

            log_syn.write("Sequence:if_stat Succeed");
            return (Stat*)new IfStat(
                    any_cast<Cond*>(vec[1]),any_cast<Stat*>(vec[3]));
        }),
while_stat({&kw_while,&cond,&kw_do,&stat},
        [](const vector<any>&vec)->any
        {
            if(vec.size()>1&&vec.size()<4)
            {
                delete any_cast<Cond*>(vec[1]);
                log_syn.write("Sequence:while_stat Failed");
                return any();
            }
            else if(vec.size()!=4)
            {
                log_syn.write("Sequence:while_stat Failed");
                return any();
            }

            log_syn.write("Sequence:while_stat Succeed");
            return (Stat*)new WhileStat(
                    any_cast<Cond*>(vec[1]),any_cast<Stat*>(vec[3]));
        }),
call_stat({&kw_call,&ident,&kw_semicolon},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=3)
            {
                log_syn.write("Sequence:call_stat Failed");
                return any();
            }

            log_syn.write("Sequence:call_stat Succeed");
            return (Stat*)new CallStat(any_cast<string>(vec[1]));
        }),
comp_stat({&kw_begin,&rep_stat,&kw_end},
        [](const vector<any>&vec)->any
        {
            if(vec.size()==2)
            {
                auto v=any_cast<vector<any>>(vec[1]);
                for(auto&x:v)delete any_cast<Stat*>(x);
                log_syn.write("Sequence:comp_stat Failed");
                return any();
            }
            else if(vec.size()!=3)
            {
                log_syn.write("Sequence:comp_stat Failed");
                return any();
            }

            auto v=any_cast<vector<any>>(vec[1]);
            auto p=new CompoundStat();
            for(auto it=v.begin();it!=v.end();++it)
                p->add_stat(any_cast<Stat*>(*it));

            log_syn.write("Sequence:comp_stat Succeed");
            return (Stat*)p;
        });

Sequence
const_def({&ident,&kw_assign,&integer},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=3)
            {
                log_syn.write("Sequence:const_def Failed");
                return any();
            }

            log_syn.write("Sequence:const_def Succeed");
            return std::make_pair(
                    any_cast<string>(vec[0]),any_cast<int>(vec[2]));
        });
RepeatSequence rep_comma_const_def({&kw_comma,&const_def},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=2)
            {
                log_syn.write("Sequence:comma_const_def Failed");
                return any();
            }

            log_syn.write("Sequence:comma_const_def Succeed");
            return vec[1];
        });
RepeatSequence const_def_part(
        {&kw_const,&const_def,&rep_comma_const_def,&kw_semicolon},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=4)
            {
                log_syn.write("Sequence:const_def_part Failed");
                return any();
            }

            vector<pair<string,int>>v;
            v.push_back(any_cast<pair<string,int>>(vec[1]));
            vector<any>v2=any_cast<vector<any>>(vec[2]);
            for(auto it=v2.begin();it!=v2.end();++it)
                v.push_back(any_cast<pair<string,int>>(*it));
            log_syn.write("Sequence:const_def_part Succeed");
            return v;
        },
        [](const vector<any>&vec)->any
        {
            log_syn.write("Repeat:const_def_part Succeed");
            if(vec.size())return vec[0];
            return vector<pair<string,int>>();
        },1);

RepeatSequence rep_comma_ident({&kw_comma,&ident},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=2)
            {
                log_syn.write("Sequence:comma_ident Failed");
                return any();
            }

            log_syn.write("Sequence:comma_ident Succeed");
            return vec[1];
        });
RepeatSequence var_def_part(
        {&kw_var,&ident,&rep_comma_ident,&kw_semicolon},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=4)
            {
                log_syn.write("Sequence:var_def_part Failed");
                return any();
            }

            vector<string>v;
            v.push_back(any_cast<string>(vec[1]));
            vector<any>v2=any_cast<vector<any>>(vec[2]);
            for(auto it=v2.begin();it!=v2.end();++it)
                v.push_back(any_cast<string>(*it));
            log_syn.write("Sequence:var_def_part Succeed");
            return v;
        },
        [](const vector<any>&vec)->any
        {
            log_syn.write("Repeat:var_def_part Succeed");
            if(vec.size())return vec[0];
            else return vector<string>();
        },1);

Sequence proc_head({&kw_proc,&ident,&kw_colon},
        [](const vector<any>&vec)->any
        {
            if(vec.size()!=3)
            {
                log_syn.write("Sequence:proc_head Failed");
                return any();
            }

            log_syn.write("Sequence:proc_head Succeed");
            return vec[1];
        });

RepeatSequence proc_def_part(
        {&proc_head,&const_def_part,&var_def_part,&proc_def_part,&stat},
        [](const vector<any>&vec)->any
        {
            if(vec.size()==4)
            {
                auto vpd=any_cast<vector<any>>(vec[3]);
                for(auto&x:vpd)delete any_cast<Proc*>(x);
                log_syn.write("Sequence:proc_def Failed");
                return any();
            }
            else if(vec.size()!=5)
            {
                log_syn.write("Sequence:proc_def Failed");
                return any();
            }

            auto p=new Proc(any_cast<string>(vec[0]),any_cast<Stat*>(vec[4]));
            auto vcd=any_cast<vector<pair<string,int>>>(vec[1]);
            for(auto&x:vcd)p->add_def(x.first,x.second);
            auto vvd=any_cast<vector<string>>(vec[2]);
            for(auto&x:vvd)p->add_var(x);
            auto vpd=any_cast<vector<any>>(vec[3]);
            for(auto&x:vpd)p->add_proc(any_cast<Proc*>(x));

            log_syn.write("Sequence:proc_def Succeed");
            return p;
        });

Sequence main_proc(
        {&const_def_part,&var_def_part,&proc_def_part,&stat},
        [](const vector<any>&vec)->any
        {
            if(vec.size()==3)
            {
                auto vpd=any_cast<vector<any>>(vec[2]);
                for(auto&x:vpd)delete any_cast<Proc*>(x);
                log_syn.write("Sequence:main_proc Failed");
                return any();
            }
            else if(vec.size()!=4)
            {
                log_syn.write("Sequence:main_proc Failed");
                return any();
            }

            auto p=new MainProc(any_cast<Stat*>(vec[3]));
            auto vcd=any_cast<vector<pair<string,int>>>(vec[0]);
            for(auto&x:vcd)p->add_def(x.first,x.second);
            auto vvd=any_cast<vector<string>>(vec[1]);
            for(auto&x:vvd)p->add_var(x);
            auto vpd=any_cast<vector<any>>(vec[2]);
            for(auto&x:vpd)p->add_proc(any_cast<Proc*>(x));

            log_syn.write("Sequence:main_proc Succeed");
            return p;
        });
}
