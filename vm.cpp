#include "vm.hpp"
using namespace vm;

Instruction VM::code[CODE_MAX];
int VM::code_len;

int VM::stack[STACK_MAX];
int VM::ip,VM::sp,VM::bp;

const char*VM::fun_names[]
{
    "NOP",
    "LIT","LOD","STO",
    "CAL","RET","INT",
    "JMP","JPC",
    "OPR"
};
const char*VM::opr_names[]
{
    "NEG",
    "ADD","SUB",
    "MUL","DIV",

    "EQ","NE",
    "GT","GE",
    "LT","LE"
};
