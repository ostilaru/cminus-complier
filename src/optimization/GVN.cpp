#include "GVN.h"

#include "BasicBlock.h"
#include "Constant.h"
#include "DeadCode.h"
#include "FuncInfo.h"
#include "Function.h"
#include "Instruction.h"
#include "logging.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

using namespace GVNExpression;
using std::string_literals::operator""s;
using std::shared_ptr;

static auto get_const_int_value = [](Value *v) { return dynamic_cast<ConstantInt *>(v)->get_value(); };
static auto get_const_fp_value = [](Value *v) { return dynamic_cast<ConstantFP *>(v)->get_value(); };
// Constant Propagation helper, folders are done for you
Constant *ConstFolder::compute(Instruction *instr, Constant *value1, Constant *value2) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::add: return ConstantInt::get(get_const_int_value(value1) + get_const_int_value(value2), module_);
    case Instruction::sub: return ConstantInt::get(get_const_int_value(value1) - get_const_int_value(value2), module_);
    case Instruction::mul: return ConstantInt::get(get_const_int_value(value1) * get_const_int_value(value2), module_);
    case Instruction::sdiv: return ConstantInt::get(get_const_int_value(value1) / get_const_int_value(value2), module_);
    case Instruction::fadd: return ConstantFP::get(get_const_fp_value(value1) + get_const_fp_value(value2), module_);
    case Instruction::fsub: return ConstantFP::get(get_const_fp_value(value1) - get_const_fp_value(value2), module_);
    case Instruction::fmul: return ConstantFP::get(get_const_fp_value(value1) * get_const_fp_value(value2), module_);
    case Instruction::fdiv: return ConstantFP::get(get_const_fp_value(value1) / get_const_fp_value(value2), module_);

    case Instruction::cmp:
        switch (dynamic_cast<CmpInst *>(instr)->get_cmp_op()) {
        case CmpInst::EQ: return ConstantInt::get(get_const_int_value(value1) == get_const_int_value(value2), module_);
        case CmpInst::NE: return ConstantInt::get(get_const_int_value(value1) != get_const_int_value(value2), module_);
        case CmpInst::GT: return ConstantInt::get(get_const_int_value(value1) > get_const_int_value(value2), module_);
        case CmpInst::GE: return ConstantInt::get(get_const_int_value(value1) >= get_const_int_value(value2), module_);
        case CmpInst::LT: return ConstantInt::get(get_const_int_value(value1) < get_const_int_value(value2), module_);
        case CmpInst::LE: return ConstantInt::get(get_const_int_value(value1) <= get_const_int_value(value2), module_);
        }
    case Instruction::fcmp:
        switch (dynamic_cast<FCmpInst *>(instr)->get_cmp_op()) {
        case FCmpInst::EQ: return ConstantInt::get(get_const_fp_value(value1) == get_const_fp_value(value2), module_);
        case FCmpInst::NE: return ConstantInt::get(get_const_fp_value(value1) != get_const_fp_value(value2), module_);
        case FCmpInst::GT: return ConstantInt::get(get_const_fp_value(value1) > get_const_fp_value(value2), module_);
        case FCmpInst::GE: return ConstantInt::get(get_const_fp_value(value1) >= get_const_fp_value(value2), module_);
        case FCmpInst::LT: return ConstantInt::get(get_const_fp_value(value1) < get_const_fp_value(value2), module_);
        case FCmpInst::LE: return ConstantInt::get(get_const_fp_value(value1) <= get_const_fp_value(value2), module_);
        }
    default: return nullptr;
    }
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value1) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::sitofp: return ConstantFP::get((float)get_const_int_value(value1), module_);
    case Instruction::fptosi: return ConstantInt::get((int)get_const_fp_value(value1), module_);
    case Instruction::zext: return ConstantInt::get((int)get_const_int_value(value1), module_);
    default: return nullptr;
    }
}

/*
这段代码定义了一个函数 print_congruence_class，它用于将给定的一个类型为 CongruenceClass 的对象转换为字符串，并打印出它的信息。

首先，该函数创建一个字符串流对象 ss。然后，它检查给定的 CongruenceClass 对象的 index_ 字段是否为 0。如果是，说明这个类是一个“top class”，这个函数会立即返回一个字符串“top class”。

如果给定的 CongruenceClass 对象的 index_ 字段不为 0，那么该函数会继续执行，将该对象的其他字段的值写入到字符串流中。例如，它会将 index_ 字段的值写入到字符串流中，并在前面添加一个字符串“index: ”，以便在最后打印输出时能够区分这些信息。

该函数会按照类似的方式处理给定的 CongruenceClass 对象的其他字段，例如 leader_、value_phi_ 和 value_expr_。最后，它会打印出该对象的 members_ 字段，这个字段是一个包含了该对象中所有成员的容器。

最后，该函数会将字符串流的内容转换为字符串，并返回。
*/
namespace utils {
static std::string print_congruence_class(const CongruenceClass &cc) {
    std::stringstream ss;
    if (cc.index_ == 0) {
        ss << "top class\n";
        return ss.str();
    }
    ss << "\nindex: " << cc.index_ << "\nleader: " << cc.leader_->print()
       << "\nvalue phi: " << (cc.value_phi_ ? cc.value_phi_->print() : "nullptr"s)
       << "\nvalue expr: " << (cc.value_expr_ ? cc.value_expr_->print() : "nullptr"s) << "\nmembers: {";
    for (auto &member : cc.members_)
        ss << member->print() << "; ";
    ss << "}\n";
    return ss.str();
}

/*
这段代码定义了一个函数 dump_cc_json，它用于将给定的一个类型为 CongruenceClass 的对象转换为 JSON 格式的字符串。

首先，该函数创建一个空字符串 json。然后，它会迭代给定的 CongruenceClass 对象的 members_ 字段，并将容器中的每个成员添加到 json 中。

对于每个成员，该函数会判断它是否为一个 Constant 类型的对象，如果是，则直接将该成员的字符串表示添加到 json 中；否则，会在该成员的名称前面添加一个字符串“%"，并将它添加到 json 中。

最后，该函数会将一个中括号添加到 json 的两端，以表示它是一个 JSON 数组
*/
static std::string dump_cc_json(const CongruenceClass &cc) {
    std::string json;
    json += "[";
    for (auto member : cc.members_) {
        if (auto c = dynamic_cast<Constant *>(member))
            json += member->print() + ", ";
        else
            json += "\"%" + member->get_name() + "\", ";
    }
    json += "]";
    return json;
}


/*
这段代码定义了一个函数 dump_partition_json()，它接收一个 GVN::partitions 类型的对象 p 作为参数。

GVN::partitions 类型是一个控制流图中连通分量（CC）的集合。函数遍历 partitions 对象中的每个 CC，并对每个 CC 调用 dump_cc_json() 来生成 CC 的 JSON 字符串表示。

每个 CC 的 JSON 字符串将被拼接在一起形成一个单独的 JSON 字符串，然后由函数返回。

这个函数的目的可能是将 partitions 对象转换为 JSON 字符串，然后用于序列化或通过网络传输。

这个函数可能是一个执行控制流图分析并使用 JSON 存储或传输结果
*/
static std::string dump_partition_json(const GVN::partitions &p) {
    std::string json;
    json += "[";
    for (auto cc : p)
        json += dump_cc_json(*cc) + ", ";
    json += "]";
    return json;
}


/*
这个函数输入一个映射，其中键为指向BasicBlock结构体的指针，值为GVN::partitions结构体。函数的作用是将这个映射转换为一个JSON格式的字符串。

首先，函数创建一个空的字符串json。然后，它循环遍历输入映射中的所有项，对于每个项，它将输入映射中对应的键（即指向BasicBlock结构体的指针）作为键，将调用函数dump_partition_json来转换为JSON格式的字符串作为值，

并将这两个字符串拼接到json字符串中。最后，函数将json字符串作为结果返回。
*/
static std::string dump_bb2partition(const std::map<BasicBlock *, GVN::partitions> &map) {
    std::string json;
    json += "{";
    for (auto [bb, p] : map)
        json += "\"" + bb->get_name() + "\": " + dump_partition_json(p) + ",";
    json += "}";
    return json;
}

/*
这个函数输入一个GVN::partitions结构体，它的作用是打印这个结构体中包含的所有元素，并以日志的形式输出到控制台。

首先，如果输入的结构体为空，函数会打印一条"empty partitions"消息并直接返回。

否则，函数会循环遍历输入结构体中的所有元素，对于每个元素，它都会调用函数print_congruence_class来将元素转换为字符串形式，并将这些字符串拼接在一起。最后，函数会将拼接后的字符串以日志的形式输出到控制台。

注意，函数使用了一个叫做LOG_DEBUG的宏来输出日志，而不是使用标准库中的std::cout流。
*/
// logging utility for you
static void print_partitions(const GVN::partitions &p) {
    if (p.empty()) {
        LOG_DEBUG << "empty partitions\n";
        return;
    }
    std::string log;
    for (auto &cc : p)
        log += print_congruence_class(*cc);
    LOG_DEBUG << log; // please don't use std::cout
}
} // namespace utils

GVN::partitions GVN::join(const partitions &P1, const partitions &P2) {
    // TODO: do intersection pair-wise
    return {};
}

std::shared_ptr<CongruenceClass> GVN::intersect(std::shared_ptr<CongruenceClass> Ci,
                                                std::shared_ptr<CongruenceClass> Cj) {
    // TODO:
    return {};
}


/*
1. 首先遍历所有的基础块，并对每个基础块中的指令进行分组。每个分组中的指令都满足两个条件：它们在同一个基础块中，且它们的类型相同。
    (例如，一个分组中可能会包含所有的加法指令，另一个分组中可能会包含所有的减法指令。)

2. 接下来，对于每个分组，遍历所有的指令，并将它们与分组中的其他指令进行比较。
    (比较的方式可能是比较指令的操作数，例如比较它们的左操作数和右操作数是否相同。如果两个指令的操作数完全相同，那么这两个指令就是等价的。)

3. 每当发现两个指令是等价的时候，就将它们添加到同一个等价类中。
    (例如，如果发现指令 A 和指令 B 是等价的，就可以将它们添加到同一个等价类中，以便以后在代码优化过程中对它们进行处理。)
*/
void GVN::detectEquivalences() {
    std::cout << "detectEquivalences()" << std::endl;
    bool changed{};
    // bool changed = false;
    // initialize pout with top
    // iterate until converge
    BasicBlock *entry = func_->get_entry_block();
    do {
        changed = false;
        // see the pseudo code in documentation
        // 注意：llvm 的链表不支持复制，所以我们需要加引用 &
        // auto &bbs = func_->get_basic_blocks();
        for (auto &bb : func_->get_basic_blocks()) { // you might need to visit the blocks in depth-first order
            // get PIN of bb by predecessor(s)
            auto p = clone(pin_[&bb]);
            // iterate through all instructions in the block
            // and the phi instruction in all the successors
            for(auto &instr : bb.get_instructions()){
                p = transferFunction(&instr, &instr, p);
            }

            // check changes in pout
            if(p != pout_[&bb]){    // 注意：这里需要重载 partition 的比较
                changed = true;
            }
            pout_[&bb] = std::move(p);
        }
    } while (changed);
}

/*
根据传入的 Instruction 对象，生成一个 GVNExpression::Expression 对象
*/
shared_ptr<Expression> GVN::valueExpr(Instruction *instr) {
    // TODO:
    // 获取指令的操作符
    auto op = instr->get_instr_type();

    // 根据操作符和操作数生成对应的 Expression
    switch (op) {
        case Instruction::OpID::add:{
            // 对于加法指令，创建一个二元表达式
            // 获取指令的操作数
            auto op1 = instr->get_operand(0);
            auto op2 = instr->get_operand(1);
            auto op1_expr = VariableExpression::create(op1);
            auto op2_expr = VariableExpression::create(op2);
            return BinaryExpression::create(op, op1_expr, op2_expr);
        }
        case Instruction::OpID::alloca: {
            // 对于 alloca 指令，直接返回一个变量表达式
            auto var_expr = VariableExpression::create(instr);
            return var_expr;
        }
        case Instruction::OpID::store: {
            // 对于 store 指令，将它的操作数作为变量表达式返回
            auto op = instr->get_operand(0);
            auto var_expr = VariableExpression::create(op);
            return var_expr;
        }
        default:
            return nullptr;
    }
    return {};
}

bool CongruenceClass::contains(Value *e) const {
  // 查找 e
  auto it = members_.find(e);
  // 如果找到了，返回 true
  if (it != members_.end()) {
    return true;
  }
  // 否则，返回 false
  return false;
}

bool CongruenceClass::contains(std::shared_ptr<GVNExpression::Expression> exper) const {
  // 如果 exper 为 nullptr，则返回 false
  if (!exper) return false;

  // 如果 value_expr_ 为 nullptr，则返回 false
  if (!value_expr_) return false;

  // 如果 exper 和 value_expr_ 指向相同的对象，则返回 true
  if (exper == value_expr_) return true;

  // 否则，返回 false
  return false;
}

bool CongruenceClass::contains(std::shared_ptr<GVNExpression::PhiExpression> phi) const {
  // 如果 phi 为 nullptr，则返回 false
  if (!phi) return false;

  // 如果 value_expr_ 为 nullptr，则返回 false
  if (!value_phi_) return false;

  // 如果 phi 和 value_expr_ 指向相同的对象，则返回 true
  if (phi == value_phi_) return true;

  // 否则，返回 false
  return false;
}

/*
这里我自己定义了一个函数，用来获取一个 value 在一个 partition 中的 congruenceClass
*/
std::shared_ptr<CongruenceClass> getClassOf(std::set<std::shared_ptr<CongruenceClass>>& partition, Value *e) {
  // 遍历 partition 中的每一个 congruenceClass
  for (const auto& congruenceClass : partition) {
    // 如果该元素属于当前的等价类，则返回该等价类
    if (congruenceClass->contains(e)) {
      return congruenceClass;
    }
  }
  // 如果没有找到包含该元素的等价类，则返回 nullptr
  return nullptr;
}

/*
重载实现对 exper 的
*/
std::shared_ptr<CongruenceClass> getClassOf(std::set<std::shared_ptr<CongruenceClass>>& partition, std::shared_ptr<GVNExpression::Expression>& exper) {
  // 遍历 partition 中的每一个元素
  for (const auto& congruenceClass : partition) {
    // 如果该指令属于当前的等价类，则返回该等价类
    if (congruenceClass->contains(exper)) {
      return congruenceClass;
    }
  }

  // 如果没有找到包含该指令的等价类，则返回 nullptr
  return nullptr;
}

/*
重载实现对 phi 的
*/
std::shared_ptr<CongruenceClass> getClassOf(std::set<std::shared_ptr<CongruenceClass>>& partition, std::shared_ptr<GVNExpression::PhiExpression>& phi) {
  // 遍历 partition 中的每一个元素
  for (const auto& congruenceClass : partition) {
    // 如果该指令属于当前的等价类，则返回该等价类
    if (congruenceClass->contains(phi)) {
      return congruenceClass;
    }
  }

  // 如果没有找到包含该指令的等价类，则返回 nullptr
  return nullptr;
}


/*
TransferFunction(x = e, PINs)
    POUTs = PINs
    if x is in a class Ci in POUTs
        then Ci = Ci − {x}
    ve = valueExpr(e)
    vpf = valuePhiFunc(ve,PINs) // can be NULL
    if ve or vpf is in a class Ci in POUTs // ignore vpf when NULL
    then
        Ci = Ci ∪ {x, ve} // set union
    else
        POUTs = POUTs ∪ {vn, x, ve : vpf} // vn is new value number
    return POUTs

transferFunction 的作用是将一条指令的输入值集合（PINs）转换为该指令的输出值集合（POUTs）

首先，将输出值集合初始化为输入值集合。然后，如果当前指令属于输出值集合中的某个类，则将该指令从类中删除

接着，计算指令的值表达式（valueExpr），并用它计算值φ函数（valuePhiFunc）。如果值表达式或值φ函数在输出值集合中的某个类，则将指令和值表达式添加到该类中。

否则，将指令、值表达式和一个新的值号添加到输出值集合中

*/
// instruction of the form `x = e`, mostly x is just e (SSA), but for copy stmt x is a phi instruction in the
// successor. Phi values (not copy stmt) should be handled in detectEquiv
/// \param bb basic block in which the transfer function is called
GVN::partitions GVN::transferFunction(Instruction *x, Value *e, partitions pin) {   // 这里助教演示中给出的实现是为每一条指令都新创建一个等价类
    partitions pout = clone(pin);
    // 创建一个新的等价类，并将它加入 POUTs 中
    // auto cc = createCongruenceClass(next_value_number_++);
    // cc->leader_ = x;
    // cc->members_ = {x};
    // cc->value_expr_ = valueExpr(e);
    // pout.insert(x);

    // 如果 x 在 POUTs 中已经存在于一个等价类中，就将它从该等价类中移除
    if (auto cc = getClassOf(pout, x)) {
        cc->members_.erase(x);
    }

    // 计算 ve 和 vpf
    auto ve = valueExpr(x);
    auto vpf = valuePhiFunc(ve, pin);

    // 如果 ve 或 vpf 已经存在于 POUTs 中的某个等价类中，
    // 就将 x 和 ve 添加到该等价类中
    if (auto cc = getClassOf(pout, ve)) {
        cc->members_.insert(x);
        cc->value_expr_ = ve;
    } else if (vpf && ( cc = getClassOf(pout, vpf))) {
        cc->members_.insert(x);
        cc->value_phi_ = vpf;
    } else {
        // 否则，新建一个等价类，并将 x 和 ve 添加到该等价类中
        auto cc = createCongruenceClass(next_value_number_++);
        cc->leader_ = x;
        cc->members_ = {x};
        cc->value_expr_ = vpf ? vpf : ve;
        pout.insert(cc);
    }

    // if e is add %op1, %op2
    // v1 represents CongruenceClass with members = {%op1,...}
    // v2 represents CongruenceClass with members = {%op2,...}
    // type of v1 v2 is inherited from Expression
    // 如果当前指令是加法指令，则计算它的值表达式
    if(x->is_add()){
        // 如果指令的操作数不是两个，说明该指令不合法，不进行处理
        if(x->get_operands().size() != 2) return pout;

        // 获取操作数
        auto op1 = x->get_operand(0);
        auto op2 = x->get_operand(1);

        // 获取操作数所在的等价类
        auto cc1 = getClassOf(pout, op1);
        auto cc2 = getClassOf(pout, op2);
        if(!cc1 || !cc2) return pout;

        // auto op1_expr = valueExpr()
        // auto op2_expr = std::make_shared<BinaryExpression>(cc2->value_expr_);

        // 更新等价类中的值表达式
        // 定义 new_expr
        // auto new_expr = BinaryExpression::create(Instruction::OpID::add, op1_expr, op2_expr);
        auto new_expr = valueExpr(x);
        cc1->value_expr_ = new_expr;
        pout.insert(cc1);
    }

    // cc->value_expr_ = (BinaryExpression){lhs=v1, rhs=v2, op=add}

    // TODO: get different ValueExpr by Instruction::OpID, modify pout
    
    return pout;
}

/*
valuePhiFunc(ve,P)
    if ve is of the form φk(vi1, vj1) ⊕ φk(vi2, vj2)
    then
        // process left edge
        vi = getVN(POUTkl, vi1 ⊕ vi2)
        if vi is NULL
        then vi = valuePhiFunc(vi1 ⊕ vi2, POUTkl)
        // process right edge
        vj = getVN(POUTkr, vj1 ⊕ vj2)
        if vj is NULL
        then vj = valuePhiFunc(vj1 ⊕ vj2, POUTkr)

    if vi is not NULL and vj is not NULL
    then return φk(vi, vj)
    else return NULL

伪代码的 valuePhiFunc 函数的作用是检查给定的指令表达式 ve 是否是一个 φ 函数，如果是，则返回一个新的值编号，否则返回 NULL。

为了处理这个检查，该函数遍历 ve 的每个操作数，并利用 POUTkl 和 POUTkr 两个集合来确定这些操作数是否已经在给定的集合 P 中被赋予了值编号。

如果一个操作数没有被赋予值编号，那么它将会被递归调用 valuePhiFunc 函数。如果 vi 和 vj 都有值编号，则函数会返回一个新的值编号来表示这个 φ 函数。否则，函数将返回 NULL。
*/
shared_ptr<PhiExpression> GVN::valuePhiFunc(shared_ptr<Expression> ve, const partitions &P) {
    // TODO:
    // 对应伪代码的第一个 if，检查 ve 是不是一个 φk(vi1, vj1) ⊕ φk(vi2, vj2) 的形式
    if(ve->get_expr_type() == Expression::gvn_expr_t::e_bin) {
        auto binary_ve = dynamic_cast<BinaryExpression *>(ve.get());
        if(binary_ve->op_ == Instruction::OpID::add){ // φk(vi1, vj1) + φk(vi2, vj2)
            auto left = binary_ve->lhs_;
            auto right = binary_ve->rhs_;
            // auto phi1 = dynamic_cast<PhiExpression *>(left->get_expr_type());
            // auto phi2 = dynamic_cast<PhiExpression *>(right->get_expr_type());
            auto phi1 = left->get_expr_type();
            auto phi2 = right->get_expr_type();
            if(phi1 == Expression::gvn_expr_t::e_phi && phi2 == Expression::gvn_expr_t::e_phi){
                // process left edge
                // 定义 new_expr
                shared_ptr<Expression> new_left_expr;
                auto left_phi = dynamic_cast<PhiExpression *>(left.get());
                auto right_phi = dynamic_cast<PhiExpression *>(right.get());
                new_left_expr = BinaryExpression::create(Instruction::OpID::add, left_phi->lhs_, right_phi->lhs_);
                auto vi = getVN(P, new_left_expr);
                if(!vi){
                    vi = valuePhiFunc(new_left_expr, P);
                }
                // process right edge
                // 定义 new_expr
                shared_ptr<Expression> new_right_expr;
                new_right_expr = BinaryExpression::create(Instruction::OpID::add, left_phi->rhs_, right_phi->rhs_);
                auto vj = getVN(P, new_right_expr);
                if(!vj){
                    vj = valuePhiFunc(new_right_expr, P);
                }
                // 对应伪代码中的 if vi is not NULL and vj is not NULL
                // 如果 vi 和 vj 都不为 nullptr，就返回一个新的 PhiExpression
                if(vi && vj){
                    return (PhiExpression::create(vi, vj));
                }
            }
        }
    }
    
    return {};
}

/*
getVN函数可能是获取给定输出集 POUT 中 vj1与vj2 操作后的结果的值号。 (ve 是传入函数的第二个参数，它应该是一个指向表达式的智能指针)

pout 是一个容器，里面存储着多个类，每个类都是表示一组等价的变量。每个类包含了一个或多个变量，还包含了等价表达式, 总之，pout 的作用是记录程序中变量的等价关系

如果这个结果在 POUT 中存在，那么函数会返回这个结果的值号；如果不存在，那么函数会返回 NULL
*/
shared_ptr<Expression> GVN::getVN(const partitions &pout, shared_ptr<Expression> ve) {
    // TODO: return what?
    for (auto it = pout.begin(); it != pout.end(); it++)    // 遍历 pout 中的所有等价类，查找第一个包含表达式 ve 的类
        if ((*it)->value_expr_ and *(*it)->value_expr_ == *ve)
            return ((*it)->value_expr_);;           // 如果找到，返回该类中的一个表达式
    return nullptr;  
}

void GVN::initPerFunction() {
    next_value_number_ = 1;
    pin_.clear();
    pout_.clear();
}

/*
这个函数用于执行代码优化的一种算法——通用值消除（GVN）。GVN 的目的是优化程序中的相同表达式，消除不必要的计算。

它通过构建一个等价类（equivalence class）来实现这个目的。每个等价类由一组相同表达式组成，它们都被赋予一个代表值，即等价类的领导者。

在这个函数中，每个等价类都有一个成员列表（cc->members_），一个领导者（cc->leader_）和一个代表值（cc->value_phi_）。

该函数的目的是替换等价类中的成员，使用它们的领导者来替换。但是，该函数只有在使用等价类的成员的用户在同一个块（bb）中时才会替换它们。这是因为，如果用户不在同一个块中，那么替换等价类成员可能会导致错误的结果。

此外，该函数还会忽略那些等价类中的成员就是领导者本身（if (member != cc->leader_)）。这是因为领导者已经是等价类的代表值，不需要再替换。

最后，该函数还有一些额外的逻辑，如果等价类的代表值（cc->value_phi_）是一个 phi 节点，或者等价类中的成员不是一个 phi 节点（!member_is_phi），那么它才会替换等价类中的成员。

这是因为 phi 节点在 GVN 中有特殊的作用，它们在不同块中传递变量的值，如果对它们进行替换可能会导致结果错误。
*/
void GVN::replace_cc_members() {
    for (auto &[_bb, part] : pout_) {
        auto bb = _bb; // workaround: structured bindings can't be captured in C++17
        for (auto &cc : part) {
            if (cc->index_ == 0)
                continue;
            // if you are planning to do constant propagation, leaders should be set to constant at some point
            for (auto &member : cc->members_) {
                bool member_is_phi = dynamic_cast<PhiInst *>(member);
                bool value_phi = cc->value_phi_ != nullptr;
                if (member != cc->leader_ and (value_phi or !member_is_phi)) {
                    // only replace the members if users are in the same block as bb
                    member->replace_use_with_when(cc->leader_, [bb](User *user) {
                        if (auto instr = dynamic_cast<Instruction *>(user)) {
                            auto parent = instr->get_parent();
                            auto &bb_pre = parent->get_pre_basic_blocks();
                            if (instr->is_phi()) // as copy stmt, the phi belongs to this block
                                return std::find(bb_pre.begin(), bb_pre.end(), bb) != bb_pre.end();
                            else
                                return parent == bb;
                        }
                        return false;
                    });
                }
            }
        }
    }
    return;
}

/*
在 GVN 算法中，PIN 和 POUT 是两个基本概念。

PIN 指的是一个基本块的输入表达式集合，即进入基本块的变量的有序列表。

POUT 指的是一个基本块的输出表达式集合，即从基本块中转移到其他基本块的变量的有序列表。

GVN 算法通过计算基本块的 PIN 和 POUT 来识别重复计算并删除无用代码。这有助于提高编译器的效率，并使生成的代码更紧凑。
*/

/*
这个函数是通用值消除（GVN）的主函数，它用来执行 GVN 的核心算法。它首先判断是否需要将 GVN 过程的中间结果输出到 JSON 文件中（dump_json_），如果需要，则打开一个输出流 gvn_json。

然后，该函数会执行一些预处理，包括创建一个常量折叠器（ConstFolder）用于优化表达式，创建一个函数信息提取器（FuncInfo）用于分析函数信息，创建一个死代码消除器（DeadCode）用于删除无用的代码。

接下来，该函数会对每个函数进行处理，首先初始化每个函数的 GVN 过程，然后检测相同表达式，建立等价类。在 GVN 过程中，会输出中间结果的调试信息，并将 GVN 过程的中间结果输出到 JSON 文件中。

最后，该函数会调用 GVN::replace_cc_members() 函数替换等价类中的成员，并调用 DeadCode 类的 run() 函数消除死代码。

最后，该函数返回 void 
*/
// top-level function, done for you
void GVN::run() {
    std::cout << "GVN::run()" << std::endl;
    std::ofstream gvn_json;
    if (dump_json_) {
        gvn_json.open("gvn.json", std::ios::out);
        gvn_json << "[";
    }

    folder_ = std::make_unique<ConstFolder>(m_);
    func_info_ = std::make_unique<FuncInfo>(m_);
    func_info_->run();
    dce_ = std::make_unique<DeadCode>(m_);
    dce_->run(); // let dce take care of some dead phis with undef

    for (auto &f : m_->get_functions()) {
        if (f.get_basic_blocks().empty())
            continue;
        func_ = &f;
        initPerFunction();
        LOG_INFO << "Processing " << f.get_name();
        detectEquivalences();
        LOG_INFO << "===============pin=========================\n";
        for (auto &[bb, part] : pin_) {
            LOG_INFO << "\n===============bb: " << bb->get_name() << "=========================\npartitionIn: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        LOG_INFO << "\n===============pout=========================\n";
        for (auto &[bb, part] : pout_) {
            LOG_INFO << "\n=====bb: " << bb->get_name() << "=====\npartitionOut: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        if (dump_json_) {
            gvn_json << "{\n\"function\": ";
            gvn_json << "\"" << f.get_name() << "\", ";
            gvn_json << "\n\"pout\": " << utils::dump_bb2partition(pout_);
            gvn_json << "},";
        }
        replace_cc_members(); // don't delete instructions, just replace them
    }
    dce_->run(); // let dce do that for us
    if (dump_json_)
        gvn_json << "]";
}

/*
这段代码定义了一个模板函数，用于比较左右两个表达式的相等性。该函数接受两个 Expression 类型的参数，并返回一个 bool 类型的值。

该函数首先使用 static_cast 将 lhs 和 rhs 强制转换为 T 类型的指针。然后，它调用 T 类型的 equiv 方法，将两个指针作为参数传递给该方法。最后，它返回 equiv 方法的返回值，即两个表达式是否相等。

总之，该函数用于比较两个表达式是否相等，并且针对指定的类型 T 进行了特定的实现。
*/
template <typename T>
static bool equiv_as(const Expression &lhs, const Expression &rhs) {
    // we use static_cast because we are very sure that both operands are actually T, not other types.
    return static_cast<const T *>(&lhs)->equiv(static_cast<const T *>(&rhs));
}

/*
这段代码定义了一个名为GVNExpression的类，它重载了==运算符，并在运算符中判断了两个参数lhs和rhs的表达式类型是否相同。

如果两个表达式的类型不同，那么它们不相等，==运算符返回false。如果两个表达式的类型相同，那么它们会被转换为不同的子类，并根据这两个表达式的类型调用相应的equiv_as函数进行比较。

如果equiv_as函数返回true，则表明这两个表达式是相等的，==运算符返回true。否则，它们不相等，==运算符返回false。
*/
bool GVNExpression::operator==(const Expression &lhs, const Expression &rhs) {
    if (lhs.get_expr_type() != rhs.get_expr_type())
        return false;
    switch (lhs.get_expr_type()) {
    case Expression::e_constant: return equiv_as<ConstantExpression>(lhs, rhs);
    case Expression::e_bin: return equiv_as<BinaryExpression>(lhs, rhs);
    case Expression::e_phi: return equiv_as<PhiExpression>(lhs, rhs);
    }
}

/*
这段代码定义了一个名为GVNExpression的类，它重载了==运算符，接受两个指向表达式的智能指针作为参数。

在运算符中，它首先检查两个指针是否都指向nullptr。如果两个指针都指向nullptr，则表示这两个表达式都是空表达式，运算符返回true。

如果两个指针中有一个不是nullptr，则运算符会检查这两个指针是否都有效。如果两个指针都有效，则运算符会将它们分别解除引用，并使用解除引用之后的指针调用上面提到的==运算符，进而比较它们指向的表达式。

如果这两个表达式相等，则运算符返回true，否则返回false。

如果两个指针中有一个无效，那么这两个表达式肯定不相等，运算符返回false。
*/
bool GVNExpression::operator==(const shared_ptr<Expression> &lhs, const shared_ptr<Expression> &rhs) {
    if (lhs == nullptr and rhs == nullptr) // is the nullptr check necessary here?
        return true;
    return lhs and rhs and *lhs == *rhs;
}

/*
补充一下智能指针：

C++ 中的智能指针是一种特殊的指针，它能够自动地管理指向的内存，从而避免内存泄漏和悬空指针等问题。

智能指针会自动地分配和释放内存，并且能够更安全地管理指针，这对于避免常见的内存管理错误来说非常有用。
*/

/*
这段代码定义了一个名为GVN的类，它包含一个叫做clone的成员函数，它接受一个名为p的参数，它是一个名为partitions的类型，表示一个容器。

在函数中，它创建了一个名为data的partitions容器，并遍历了原始容器p中的所有元素。对于每个遍历到的元素，它都创建一个新的CongruenceClass对象，并使用它来构造一个智能指针，最后将这个智能指针插入到data容器中。

在遍历完所有元素之后，函数返回data容器。

简单来说，这个函数的作用是将原始容器p中的所有元素复制到一个新的容器中，并返回这个新容器。
*/
GVN::partitions GVN::clone(const partitions &p) {
    partitions data;
    for (auto &cc : p) {
        data.insert(std::make_shared<CongruenceClass>(*cc));
    }
    return data;
}

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2) {
    // TODO: how to compare partitions?
    if (p1.size() != p2.size()) return false;
    // 比较两个 partitions 中的每个 CongruenceClass
    auto it1 = p1.begin();
    auto it2 = p2.begin();
    while (it1 != p1.end() && it2 != p2.end()) {
        if ((*it1)->members_ != (*it2)->members_) return false;
        ++it1;
        ++it2;
    }

    // 如果两个 partitions 中的每个 CongruenceClass 都相同，就返回 true
    return true;    
}

bool CongruenceClass::operator==(const CongruenceClass &other) const {
    // TODO: which fields need to be compared?
    return (value_expr_ == other.value_expr_) && (value_phi_ == other.value_phi_) && (members_ == other.members_);

    // return false;
}
