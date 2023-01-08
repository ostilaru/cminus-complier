/*
 * 声明：本代码为 2020 秋 中国科大编译原理（李诚）课程实验参考实现。
 * 请不要以任何方式，将本代码上传到可以公开访问的站点或仓库
 */

#include "cminusf_builder.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())


// TODO: Global Variable Declarations 
// You can define global variables here
// to store state. You can expand these
// definitions if you need to.
// 定义全局变量来存储状态等

// 存储当前value 临时变量 (值)
Value *tmp_val = nullptr;
// 是否需要 左值
bool require_lvalue = false;
// 表示是否在之前已经进入scope，用于CompoundStmt
// 进入CompoundStmt不仅包括通过Fundeclaration进入，也包括selection-stmt等。
// pre_enter_scope用于控制是否在CompoundStmt中添加scope.enter,scope.exit
bool pre_enter_scope = false;

// function that is being built
// 当前函数
Function *cur_fun = nullptr;

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

/* 由 .cpp 生成 .ll 的过程可以理解为：自顶向下遍历抽象语法树的每一个结点，
 * 利用访问者模式，调用我们写的成员函数，对每一个抽象语法树的结点进行分析，
 * 使之符合 cminusf 的语法和语义规则。
 */

void CminusfBuilder::visit(ASTProgram &node) { // 抽象语法生成树上的一个结点（这里是根节点）
    std::cout << "ASTProgram" << std::endl;
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());

    for (auto decl : node.declarations) { // program -> declaration-list
        // ASTProgram 这个结点是根结点，向下遍历(accept)它的孩子
        decl->accept(*this);// 进入下一层函数
    }
}

void CminusfBuilder::visit(ASTNum &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // ASTNum 有 int 和 float
    std::cout << "ASTNum" << std::endl;
    if(node.type == TYPE_INT)   // 调用上面的宏定义
        tmp_val = CONST_INT(node.i_val);
    else
        tmp_val = CONST_FP(node.f_val); 
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // 变量声明有两种，一种是类型+标识符，另一种是类型+标识符数组
    // var-declaration -> type-specifier ID; | type-specifier ID [INTEGER];
    std::cout << "ASTVarDeclaration" << std::endl;
    Type *var_type;
    if(node.type == TYPE_INT)   // 1. 变量
        var_type = Type::get_int32_type(module.get());
    else
        var_type = Type::get_float_type(module.get());
    
    if(node.num == nullptr){    // 声明时变量还未被创建，没有值，那么 node.num = nullptr
        if(scope.in_global()){
            // 需要用 scope.in_global()来区分变量是在全局域还是局部
            // 根据 cminus-f 语义要求，全局变量需要初始化为 0
            auto initializer = ConstantZero::get(var_type, module.get());
            auto var = GlobalVariable::create(node.id, module.get(), var_type, false, initializer);
            scope.push(node.id, var);   // 把结点入栈(id, var)
        }
        else{// 是局部变量
            auto var = builder->create_alloca(var_type);
            scope.push(node.id, var);
        }
    }
    else{ // 2. 数组
        auto *array_type = ArrayType::get(var_type, node.num->i_val);
        if(scope.in_global()){
            // 需要用 scope.in_global()来区分变量是在全局域还是局部
            // 根据 cminus-f 语义要求，全局变量需要初始化为 0
            auto initializer = ConstantZero::get(var_type, module.get());
            auto var = GlobalVariable::create(node.id, module.get(), array_type, false, initializer);
            scope.push(node.id, var);   // 把结点入栈(id, var)
        }
        else{
            auto var = builder->create_alloca(array_type);
            scope.push(node.id, var);
        }
    }

}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    // fun-declaration -> type-specifier ID (params) compound-stmt
    // 一共有两个子域
    // 1. 维护函数的参数及其返回值类型
    // 2. 维护函数输入的参数值和函数内部语句
    std::cout << "FunDeclaration" << std::endl;
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)// 函数返回值类型
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;
    // 分析函数参数的类型，并把参数 push 到作用域中 
    // 注意如果参数是数组，则为数组引用传递（指针），且不需要考虑数组下标是否合法，
    // 因为根据 cminus-f 语法，这里的数组不会有下标。

    for (auto &param : node.params) {
        //!TODO: Please accomplish param_types.
        // 根据param的类型分类
        // 需要考虑int型数组，int型，float型数组，float型
        // 对于不同的类型，向param_types中添加不同的Type
        // param_types.push_back
        if(param->type == TYPE_INT){
            if(param->isarray){ // 是 int数组，push 数组指针
                param_types.push_back(INT32PTR_T);
            }
            else{   // int型
                param_types.push_back(INT32_T);
            }
        }
        else{
            if(param->isarray){ // 是 float 数组，push 数组指针
                param_types.push_back(FLOATPTR_T);
            }
            else{
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto fun = Function::create(fun_type, node.id, module.get());//定义函数变量
    scope.push(node.id, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);//创建基本块
    builder->set_insert_point(funBB);
    scope.enter();
    std::vector<Value *> args;
    for (auto arg = fun->arg_begin(); arg != fun->arg_end(); arg++) {
        args.push_back(*arg);
    }
    for (int i = 0; i < node.params.size(); ++i) {
        //!TODO: You need to deal with params
        // and store them in the scope.
        //需要考虑int型数组，int型，float型数组，float型
        //builder->create_alloca创建alloca语句
        //builder->create_store创建store语句
        //scope.push
        if(node.params[i]->isarray){    // 数组类型
            Value *array_alloc;
            if(node.params[i]->type == TYPE_INT) // int 数组
                array_alloc = builder->create_alloca(INT32PTR_T);
            else    // float 数组
                array_alloc = builder->create_alloca(FLOATPTR_T);
            builder->create_store(args[i], array_alloc); // store
            scope.push(node.params[i]->id, array_alloc); // push
        }
        else{
            Value *alloc;
            if(node.params[i]->type == TYPE_INT) // int 型
                alloc = builder->create_alloca(INT32_T);
            else // float型
                alloc = builder->create_alloca(FLOAT_T);
            builder->create_store(args[i], alloc); // store
            scope.push(node.params[i]->id, alloc);
        }


    }
    node.compound_stmt->accept(*this);//fun-declaration -> type-specifier ID ( params ) compound-stmt
    if (builder->get_insert_block()->get_terminator() == nullptr) //创建ret语句
    {
        if (cur_fun->get_return_type()->is_void_type()) // return_type = void_type
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type()) // return_type = float_type
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0)); // return_type = int_type
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // param -> type-specifier ID | type-specifier ID[]
    std::cout << "ASTParam" << std::endl;
    Type *Int32Type = Type::get_int32_type(module.get());
    Type *FloatType = Type::get_float_type(module.get());
    Type *VoidType = Type::get_void_type(module.get());
    auto Int32PtrType = Type::get_int32_ptr_type(module.get());
    auto FloatPtrType = Type::get_float_ptr_type(module.get());

    Value *pAlloc; // alloca some memory for this param
    if(node.type == TYPE_VOID){
        // this param is void
        return;
    }
    else if(node.isarray){
        // this param is an array
        auto arrayType = node.type == TYPE_INT ? Int32PtrType : FloatPtrType;
        pAlloc = builder->create_alloca(arrayType);
        scope.push(node.id, pAlloc);
    }
    else{
        // the param is just an id
        auto idType = node.type == TYPE_INT ? Int32Type : FloatType;
        pAlloc = builder->create_alloca(idType);
        scope.push(node.id, pAlloc);
    }
}

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    //!TODO: This function is not complete.
    // You may need to add some code here
    // to deal with complex statements. 
    // 此函数为完整实现
    // compound-stmt -> {local-declarations statement-list}
    std::cout << "ASTCompoundStmt" << std::endl;
    bool need_exit_scope = !pre_enter_scope;//添加need_exit_scope变量
    if (pre_enter_scope) {
        pre_enter_scope = false;
    } else {
        scope.enter();
    }


    for (auto &decl : node.local_declarations) {//compound-stmt -> { local-declarations statement-list }
        decl->accept(*this);
    }

    for (auto &stmt : node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->get_terminator() != nullptr)
            break;
    }

    if(need_exit_scope){
        scope.exit();
    }
}

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // 表达式语句
    // expression-stmt -> expression ; | ;
    std::cout << "ASTExpressionStmt" << std::endl;
    if(node.expression != nullptr)
        node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // 选择语句
    // selection-stmt -> if (expression) statement | if (expression) statement else statement
    // 首先需要向下遍历 if 的判据（expression)：expression 类型有（int、float）指针，int，float，bool
    // 是指针类型就把值 load 出来，是 bool 就强制转化为 int32 类型，那么最后都会转化为 int32 或者 float
    // 用 cmp 与 0 比较得到分支判据，这是 expression 的处理思路，下面的 iterationstmt 是相似的
    // 其次还需要检测 selection stmt 展开之后是什么类型，有没有 else stmt，
    // 如果有那么就创建真假分支后创建跳出分支，如果没有只需要真分支和跳出分支
    // 为了解决 if 嵌套 if，以及下面的 while 嵌套 while，创建 Basicblock 时都使用默认的寄存器编号作为名字，以免分支重名
    std::cout << "ASTSelectionStmt" << std::endl;
    node.expression->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun); // trueBB
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun);// falseBB
    auto contBB = BasicBlock::create(module.get(), "", cur_fun); // contBB
    Value *cond_val;
    if(ret_val->get_type()->is_integer_type()) // int 型
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0)); // conditon <= 0 ?
    else // float 型
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));

    if(node.else_statement == nullptr){ // 没有 else
        builder->create_cond_br(cond_val, trueBB, contBB); // 先 create_br
    }
    else{
        builder->create_cond_br(cond_val, trueBB, falseBB);
    }
    // trueBB
    builder->set_insert_point(trueBB);
    node.if_statement->accept(*this);
    if(builder->get_insert_block()->get_terminator() == nullptr) // 遍历完 tureBB
        builder->create_br(contBB); // create_br -> contBB
    if(node.else_statement == nullptr){ // 没有 else
        falseBB->erase_from_parent();
    }
    else{ // 有 else
        builder->set_insert_point(falseBB); // falseBB
        node.else_statement->accept(*this);
        if(builder->get_insert_block()->get_terminator() == nullptr) // 遍历完 falseBB
            builder->create_br(contBB); // create_br -> contBB
    }
    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // iteration-stmt -> while (expression) statement
    std::cout << "ASTIterationStmt" << std::endl;
    auto exprBB = BasicBlock::create(module.get(), "", cur_fun);
    if(builder->get_insert_block()->get_terminator() == nullptr)
        builder->create_br(exprBB);
    builder->set_insert_point(exprBB); // 进入 exprBB
    node.expression->accept(*this); // 遍历 exprBB
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);

    Value *cond_val;
    if(ret_val->get_type()->is_integer_type()) // 检查返回值类型，进行相应的转化
        cond_val = builder->create_icmp_ne(ret_val, CONST_INT(0));
    else
        cond_val = builder->create_fcmp_ne(ret_val, CONST_FP(0.));
    
    builder->create_cond_br(cond_val, trueBB, contBB);
    builder->set_insert_point(trueBB); // 进入 trueBB
    node.statement->accept(*this);     // 遍历
    if(builder->get_insert_block()->get_terminator() == nullptr) // 遍历完trueBB
        builder->create_br(exprBB);    // 进入 exprBB 判断
    builder->set_insert_point(contBB);
}

void CminusfBuilder::visit(ASTReturnStmt &node) {//return-stmt -> return ; | return expression ;
    std::cout << "ASTReturnStmt" << std::endl;
    if (node.expression == nullptr) {
        builder->create_void_ret();// return void直接创建空返回
    } else {
        //!TODO: The given code is incomplete.
        // You need to solve other return cases (e.g. return an integer).
        // 需要考虑类型转换
        // 函数返回值和表达式值类型不同时，转换成函数返回值的类型
        // 用cur_fun获取当前函数返回值类型 
        // 类型转换：builder->create_fptosi
        // ret语句
        // 由于 expression 可能类型有多种，如果是指针类型则需要 load 成一个值
        // 如果 return void 那么直接创建空返回
        // 注意需要用 builder->get_insert_block()->get_parent()->get_return_type() 来检测返回值类型，不同则需要强制转换
        
        auto fun_ret_type = cur_fun->get_function_type()->get_return_type();
        node.expression->accept(*this);
        if(fun_ret_type != tmp_val->get_type()){ // 检测返回值类型
            if(fun_ret_type->is_integer_type()){ 
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);// int型的强制转化
            }
            else{
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);// float型强制转化
            }
        } 
        builder->create_ret(tmp_val);
    }
}

void CminusfBuilder::visit(ASTVar &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // var -> ID | ID[expression]
    // 首先需要在 scope 中找到 id
    // 因为 var 可能会被展开成单纯的 ID 或者 ID[expression]，即数组类型
    // 那么就需要根据 是否有 expression 进行讨论
    // 如果是纯 id，直接返回 id 地址即可
    // 如果不是，那么就是数组类型，即 ID[expression]
    // expression 对于最终的数组下标而言，需要的是一个 int32 ，所有把他们全部转化为 int32
    // 由于数组下标非负，那么需要创建一个分支去判断是否大于 0
    // id 的类型可能是：int**, float**, int*, float*, 后面两个是普通的整数和浮点数组类型
    // 对于 int** 和 float**类型，本质上是一个指针，指向了一块内部空间，这个内存空间存放了数组首地址
    // 如果数组下标小于 0，那么调用 neg_idx_except 报错
    std::cout << "ASTVar" << std::endl;
    auto var = scope.find(node.id); // 先需要在 scope 中找到 id
    assert(var != nullptr);
    auto is_int = var->get_type()->get_pointer_element_type()->is_integer_type();
    auto is_float = var->get_type()->get_pointer_element_type()->is_float_type();
    auto is_ptr = var->get_type()->get_pointer_element_type()->is_pointer_type();
    auto should_return_value = require_lvalue;
    require_lvalue = false;

    if(node.expression == nullptr){ // 纯 ID
        if(should_return_value){
            tmp_val = var;
            require_lvalue = false;
        }
        else{ // id 的类型可能是：int**, float**, int*, float*
            if(is_int || is_float || is_ptr) // 是普通的 int，float，ptr 型，直接 load 即可
                tmp_val = builder->create_load(var);
            else
                tmp_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)}); // 用 create_gep 取出内存空间中存放的数组首地址
        }
    }
    else{ // 数组
        node.expression->accept(*this);
        auto val = tmp_val; // val 实际上取的是数组下标
        Value *is_neg;
        auto exceptBB = BasicBlock::create(module.get(), "", cur_fun);
        auto contBB = BasicBlock::create(module.get(), "", cur_fun);
        if(val->get_type()->is_float_type())
            val = builder->create_fptosi(val, INT32_T); // 强制转化 float -> int（数组下标必须是 int）
        
        is_neg = builder->create_icmp_lt(val, CONST_INT(0)); // 数组下标不能小于 0

        builder->create_cond_br(is_neg, exceptBB, contBB); // 判断数组下标是否合法，即 is_neg 是否为 0
        builder->set_insert_point(exceptBB); // 数组下标合法，进入 exceptBB
        auto neg_idx_except_fun = scope.find("neg_idx_except");
        builder->create_call(static_cast<Function *>(neg_idx_except_fun), {});
        if(cur_fun->get_return_type()->is_void_type()) // void type
            builder->create_void_ret();
        else if(cur_fun->get_return_type()->is_float_type()) // float type
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0)); // int type
        
        builder->set_insert_point(contBB);
        Value *tmp_ptr;
        if(is_int || is_float)
            tmp_ptr = builder->create_gep(var, {val}); // int 和 float直接 create_gep
        else if(is_ptr){
            auto array_load = builder->create_load(var); // ptr 需要先 load 再 create_gep
            tmp_ptr = builder->create_gep(array_load, {val});
        }
        else
            tmp_ptr = builder->create_gep(var, {CONST_INT(0), val});
        if(should_return_value){
            tmp_val = tmp_ptr;
            require_lvalue = false;
        }
        else{
            tmp_val = builder->create_load(tmp_ptr);
        }
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // 赋值语句
    // expression -> var = expression | simple-expression
    // 把 expression 的值赋给 var
    // var 可以是一个 int，float 型变量，也可以是一个取了下标的数组变量
    // 通过全局 ret 获取的 expression 的返回值有：int，float，int*，float*（表示的是存放 expression 值的地址），int1，五种类型
    // 需要分别进行讨论
    // 在 expression_value 和 var_value 的类型不同时，需要以 var_value_type 为标准，对 expression_value 做强制转换
    std::cout << "ASTAssignExpression" << std::endl;
    node.expression->accept(*this);
    auto expr_result = tmp_val;
    require_lvalue = true;
    node.var->accept(*this);
    auto var_addr = tmp_val;
    if(var_addr->get_type()->get_pointer_element_type() != expr_result->get_type()){ // 类型不同，需要做强制转换
        if(expr_result->get_type() == INT32_T)
            expr_result = builder->create_sitofp(expr_result, FLOAT_T);
        else 
            expr_result = builder->create_fptosi(expr_result, INT32_T);
    }
    builder->create_store(expr_result, var_addr); // 赋值
    tmp_val = expr_result;
}


void CminusfBuilder::visit(ASTSimpleExpression &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // simple-expression -> additive-expression relop additive-expression | additive-expression
    // 浮点数和整数一起运算时，需要对整数值进行类型提升，转化成浮点类型，其运算结果也为浮点类型
    std::cout << "ASTSimpleExpression" << std::endl;
    if(node.additive_expression_r == nullptr){
        node.additive_expression_l->accept(*this);
    }
    else{
        node.additive_expression_l->accept(*this);
        auto l_val = tmp_val;
        node.additive_expression_r->accept(*this);
        auto r_val = tmp_val;
        // bool is_int = promote(builder, &l_val, &r_val); // 类型提升
        bool is_int;
        
        if(l_val->get_type() == r_val->get_type()){ // 检测左右值类型是否一致
            is_int = l_val->get_type()->is_integer_type(); // 一致的话，若都是 int 则返回 1，都是 float 则返回 0
        }
        else{
            is_int = false; // 左右值类型一致，那么一定要提升到 float，此时返回 0
            if(l_val->get_type()->is_integer_type())
                l_val = builder->create_sitofp(l_val, FLOAT_T); // 左值是 int 那么就把左值提升到 float
            else
                r_val = builder->create_sitofp(r_val, FLOAT_T);
        }

        Value *cmp;
        switch(node.op){
            case OP_LT:
                if(is_int)
                    cmp = builder->create_icmp_lt(l_val, r_val);
                else
                    cmp = builder->create_fcmp_lt(l_val, r_val);
                break;
            case OP_LE:
                if(is_int)
                    cmp = builder->create_icmp_le(l_val, r_val);
                else
                    cmp = builder->create_fcmp_le(l_val, r_val);
                break;
            case OP_GE:
                if(is_int)
                    cmp = builder->create_icmp_ge(l_val, r_val);
                else
                    cmp = builder->create_fcmp_ge(l_val, r_val);
                break;
            case OP_GT:
                if(is_int)
                    cmp = builder->create_icmp_gt(l_val, r_val);
                else
                    cmp = builder->create_fcmp_gt(l_val, r_val);
                break;
            case OP_EQ:
                if(is_int)
                    cmp = builder->create_icmp_eq(l_val, r_val);
                else
                    cmp = builder->create_fcmp_eq(l_val, r_val);
                break;
            case OP_NEQ:
                if(is_int)
                    cmp = builder->create_icmp_ne(l_val, r_val);
                else
                    cmp = builder->create_fcmp_ne(l_val, r_val);
                break;
        }
        tmp_val = builder->create_zext(cmp, INT32_T);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // additive-expression -> additive-expression addop term | term
    // 如果生成 term，直接调用 accept，完成访问者遍历并跳转到子节点
    // 如果是 additive-expression addop term，那么要对加减法符号两端操作数进行类型转换
    // 最后根据操作数是 int 还是 float 来调用相应的加减法函数
    std::cout << "ASTAdditiveExpression" << std::endl;
    if(node.additive_expression == nullptr){
        node.term->accept(*this);
    }
    else{
        node.additive_expression->accept(*this);
        auto l_val = tmp_val;
        node.term->accept(*this);
        auto r_val = tmp_val;

        bool is_int;
        // bool is_int = promote(builder, &l_val, &r_val);
        // bool is_int = promote(builder, &l_val, &r_val);

        if(l_val->get_type() == r_val->get_type()){ // 检测左右值类型是否一致
            is_int = l_val->get_type()->is_integer_type(); // 一致的话，若都是 int 则返回 1，都是 float 则返回 0
        }
        else{
            is_int = false; // 左右值类型一致，那么一定要提升到 float，此时返回 0
            if(l_val->get_type()->is_integer_type())
                l_val = builder->create_sitofp(l_val, FLOAT_T); // 左值是 int 那么就把左值提升到 float
            else
                r_val = builder->create_sitofp(r_val, FLOAT_T);
        }

        switch(node.op){
            case OP_PLUS:
                if(is_int)
                    tmp_val = builder->create_iadd(l_val, r_val);
                else
                    tmp_val = builder->create_fadd(l_val, r_val);
                break;
            case OP_MINUS:
                if(is_int)
                    tmp_val = builder->create_isub(l_val, r_val);
                else
                    tmp_val = builder->create_fsub(l_val, r_val);
                break;
        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // term->term mulop factor | factor
    // term 和 additive_expression 的产生式形式是相同的，处理方式也是几乎一致的
    std::cout << "ASTTerm" << std::endl;
    if(node.term == nullptr){
        node.factor->accept(*this);
    }
    else{
        node.term->accept(*this);
        auto l_val = tmp_val;
        node.factor->accept(*this);
        auto r_val = tmp_val;

        bool is_int;
        // bool is_int = promote(builder, &l_val, &r_val);
        // bool is_int = promote(builder, &l_val, &r_val);

        if(l_val->get_type() == r_val->get_type()){ // 检测左右值类型是否一致
            is_int = l_val->get_type()->is_integer_type(); // 一致的话，若都是 int 则返回 1，都是 float 则返回 0
        }
        else{
            is_int = false; // 左右值类型一致，那么一定要提升到 float，此时返回 0
            if(l_val->get_type()->is_integer_type())
                l_val = builder->create_sitofp(l_val, FLOAT_T); // 左值是 int 那么就把左值提升到 float
            else
                r_val = builder->create_sitofp(r_val, FLOAT_T);
        }

        switch(node.op){
            case OP_MUL:
                if(is_int)
                    tmp_val = builder->create_imul(l_val, r_val);
                else    
                    tmp_val = builder->create_fmul(l_val, r_val);
                break;
            case OP_DIV:
                if(is_int)
                    tmp_val = builder->create_isdiv(l_val, r_val);
                else    
                    tmp_val = builder->create_fdiv(l_val, r_val);
                break;
        }
    }
}

void CminusfBuilder::visit(ASTCall &node) {
    //!TODO: This function is empty now.
    // Add some code here.
    // call -> ID(args)
    // call 分为两个部分，函数名的处理，函数传入参数列表的处理
    // 对于函数名，需要进入对应的作用域里面，并取出来函数声明时候的形参列表（用于检查和传入参数是否一致）
    // 对于传入参数，需要将指针/数组元素对应的值“取出来”，存入 vector 中，并与声明时的参数进行类型比较
    // 注意：1.传入的参数可以为空  2.调用时的实参如果与声明时的形参类型或个数不一致，需要强制转换
    std::cout << "ASTCall" << std::endl;
    auto fun = static_cast<Function *>(scope.find(node.id)); // 先取出函数名 fun
    std::vector<Value *> args;
    auto param_type = fun->get_function_type()->param_begin(); // 遍历参数列表
    for(auto &arg : node.args){
        arg->accept(*this);
        if(!tmp_val->get_type()->is_pointer_type() && *param_type != tmp_val->get_type()){ // 类型不一致，进行强制转换
            if(tmp_val->get_type()->is_integer_type()) 
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            else
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
        }
        args.push_back(tmp_val); // 放入传入参数vector
        param_type++;   // 迭代器++，下一个参数
    }
    tmp_val = builder->create_call(static_cast<Function *>(fun), args);
}
