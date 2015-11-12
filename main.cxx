#include "vm.hpp"
using namespace vm;

#include "semantic.hpp"
using namespace semantic;

#include "syntax.hpp"
using namespace syntax;

#include "reader.hpp"

int main(int argc,char**argv)
{
    const char*path="src";
    if(argc>1)path=argv[1];
    Reader rd(path);
    any val=main_proc.match(rd);
    if(val.empty())
    {
        std::cout<<"empty match"<<std::endl;
        return 0;
    }
    auto proc=any_cast<MainProc*>(val);
    proc->print(0);
    proc->translate();
    VM::print();
    delete proc;

    return 0;
}
