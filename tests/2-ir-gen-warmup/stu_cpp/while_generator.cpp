#include "../../../include/lightir/BasicBlock.h"
#include "../../../include/lightir/Constant.h"
#include "../../../include/lightir/Function.h"
#include "../../../include/lightir/IRBuilder.h"
#include "../../../include/lightir/Module.h"
#include "../../../include/lightir/Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl; // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) ConstantInt::get(num, module)

#define CONST_FP(num) ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main(){
    auto module = new Module("Cminus code");  // module name是什么无关紧要
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    Type *FloatType = Type::get_float_type(module);

    // main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    
    // BB的名字在生成中无所谓,但是可以方便阅读
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb); // 一个BB的开始,将当前插入指令点的位置设在bb

    auto retAlloca = builder->create_alloca(Int32Type);  // 在内存中分配返回值的位置
    builder->create_store(CONST_INT(0), retAlloca); // 默认 ret 0

    auto aAlloca = builder->create_alloca(Int32Type);   // 在内存中分配a的位置
    builder->create_store(CONST_INT(10), aAlloca);  // a = 10

    auto iAlloca = builder->create_alloca(Int32Type);   // 在内存中分配i的位置
    builder->create_store(CONST_INT(0), iAlloca);  // i = 0

    auto aLoad = builder->create_load(aAlloca); // 将a load上来
    auto iLoad = builder->create_load(iAlloca); // 将i load上来

    auto loop = BasicBlock::create(module, "loop", mainFun);
    builder->create_br(loop);   // br loop
    builder->set_insert_point(loop); // the entry for loop

    iLoad = builder->create_load(iAlloca);  // 将参数i load上来
    auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));  // 将i与10比较

    auto trueBB = BasicBlock::create(module, "tureBB", mainFun); // inside while
    auto falseBB = BasicBlock::create(module, "falseBB", mainFun); // after while
    auto retBB = BasicBlock::create(module, "", mainFun); // return 分支

    auto br = builder->create_cond_br(icmp, trueBB, falseBB);   // 条件BR
    DEBUG_OUTPUT

    builder->set_insert_point(trueBB);  // if true; 分支的开始需要SetInsertPoint设置
    
    iLoad = builder->create_load(iAlloca);  // 将参数i load上来
    auto addi = builder->create_iadd(iLoad, CONST_INT(1));  // add result for i
    builder->create_store(addi, iAlloca);

    aLoad = builder->create_load(aAlloca);  // 将参数a load上来
    iLoad = builder->create_load(iAlloca);  // 将参数i load上来
    auto adda = builder->create_iadd(iLoad, aLoad); // a + i
    builder->create_store(adda, aAlloca);
    builder->create_br(loop);   // br loop
    builder->set_insert_point(loop);    // the entry for loop

    builder->set_insert_point(falseBB); // after while
    aLoad = builder->create_load(aAlloca);  // 将参数a load上来
    builder->create_store(aLoad, retAlloca);
    builder->create_br(retBB);

    builder->set_insert_point(retBB);   // ret 分支
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    std::cout << module->print();
    delete module;
    return 0;
}