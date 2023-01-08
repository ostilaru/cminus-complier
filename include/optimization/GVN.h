#pragma once
#include "BasicBlock.h"
#include "Constant.h"
#include "DeadCode.h"
#include "FuncInfo.h"
#include "Function.h"
#include "Instruction.h"
#include "Module.h"
#include "PassManager.hpp"
#include "Value.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

/*
这段代码定义了一个类名为 ConstFolder，它是在命名空间 GVNExpression 中的。

该类有两个公开的成员函数：compute，它接受三个参数：一个指向指令的指针、一个指向常量的指针和另一个指向常量的指针；另一个 compute 函数接受两个参数：一个指向指令的指针和一个指向常量的指针。

该类还有一个私有成员变量 module_，它指向模块。这个类的用途是对指令执行折叠操作，即将指令中的常量值折叠为一个单独的常量。

这样做的目的是为了优化程序的性能，因为对于一些指令，比如数学运算指令，它们的结果是可以预先计算的，并不需要在运行时才能确定。

例如，如果指令是 a + b，并且 a 和 b 都是常量，那么就可以预先计算出这个指令的结果，并将其保存为一个单独的常量。这样就可以在程序运行时节省时间，因为不需要再进行计算了。
*/
namespace GVNExpression {

// fold the constant value
class ConstFolder {
  public:
    ConstFolder(Module *m) : module_(m) {}
    Constant *compute(Instruction *instr, Constant *value1, Constant *value2);
    Constant *compute(Instruction *instr, Constant *value1);

  private:
    Module *module_;
};

/**
 * for constructor of class derived from `Expression`, we make it public
 * because `std::make_shared` needs the constructor to be publicly available,
 * but you should call the static factory method `create` instead the constructor itself to get the desired data
 */
class Expression {
  public:
    // TODO: you need to extend expression types according to testcases
    enum gvn_expr_t { e_constant, e_bin, e_phi, e_var };
    Expression(gvn_expr_t t) : expr_type(t) {}
    virtual ~Expression() = default;
    virtual std::string print() = 0;
    gvn_expr_t get_expr_type() const { return expr_type; }
    // Expression *get_expr_type() const { return this; }

  private:
    gvn_expr_t expr_type;
};

bool operator==(const std::shared_ptr<Expression> &lhs, const std::shared_ptr<Expression> &rhs);
bool operator==(const GVNExpression::Expression &lhs, const GVNExpression::Expression &rhs);

// 常量
class ConstantExpression : public Expression {
  public:
    static std::shared_ptr<ConstantExpression> create(Constant *c) { return std::make_shared<ConstantExpression>(c); }
    virtual std::string print() { return c_->print(); }
    // we leverage the fact that constants in lightIR have unique addresses
    bool equiv(const ConstantExpression *other) const { return c_ == other->c_; }
    ConstantExpression(Constant *c) : Expression(e_constant), c_(c) {}

  private:
    Constant *c_;
};

// 变量
class VariableExpression : public Expression {
  public:
    static std::shared_ptr<VariableExpression> create(Value *v) { return std::make_shared<VariableExpression>(v); }
    virtual std::string print() { return v_->print(); }
    bool equiv(const VariableExpression *other) const { return v_ == other->v_; }
    VariableExpression(Value *v) : Expression(e_var), v_(v) {}
  private:
    Value *v_;
};

// arithmetic expression
// 二元运算
class BinaryExpression : public Expression {
  public:
    static std::shared_ptr<BinaryExpression> create(Instruction::OpID op,
                                                    std::shared_ptr<Expression> lhs,
                                                    std::shared_ptr<Expression> rhs) {
        return std::make_shared<BinaryExpression>(op, lhs, rhs);
    }
    virtual std::string print() {
        return "(" + Instruction::get_instr_op_name(op_) + " " + lhs_->print() + " " + rhs_->print() + ")";
    }

    bool equiv(const BinaryExpression *other) const {
        if (op_ == other->op_ and *lhs_ == *other->lhs_ and *rhs_ == *other->rhs_)
            return true;
        else
            return false;
    }

    BinaryExpression(Instruction::OpID op, std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs)
        : Expression(e_bin), op_(op), lhs_(lhs), rhs_(rhs) {}

  // private:
    Instruction::OpID op_;
    std::shared_ptr<Expression> lhs_, rhs_;
};

// phi 函数
class PhiExpression : public Expression {
  public:
    static std::shared_ptr<PhiExpression> create(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs) {
        return std::make_shared<PhiExpression>(lhs, rhs);
    }
    virtual std::string print() { return "(phi " + lhs_->print() + " " + rhs_->print() + ")"; }
    bool equiv(const PhiExpression *other) const {
        if (*lhs_ == *other->lhs_ and *rhs_ == *other->rhs_)
            return true;
        else
            return false;
    }
    PhiExpression(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs)
        : Expression(e_phi), lhs_(lhs), rhs_(rhs) {}

  // private:
    std::shared_ptr<Expression> lhs_, rhs_;
};
} // namespace GVNExpression

/**
 * Congruence class in each partitions
 * note: for constant propagation, you might need to add other fields
 * and for load/store redundancy detection, you most certainly need to modify the class
 */
struct CongruenceClass {
    size_t index_;
    // representative of the congruence class, used to replace all the members (except itself) when analysis is done
    Value *leader_;
    // value expression in congruence class
    std::shared_ptr<GVNExpression::Expression> value_expr_; // 值表达式
    // value φ-function is an annotation of the congruence class
    std::shared_ptr<GVNExpression::PhiExpression> value_phi_; // φ函数
    // equivalent variables in one congruence class
    std::set<Value *> members_;

    // 这里自己实现了一个 contains 成员函数
    bool contains(Value *e) const;
    bool contains(std::shared_ptr<GVNExpression::Expression> exper) const;
    bool contains(std::shared_ptr<GVNExpression::PhiExpression> phi) const;

    CongruenceClass(size_t index) : index_(index), leader_{}, value_expr_{}, value_phi_{}, members_{} {}

    bool operator<(const CongruenceClass &other) const { return this->index_ < other.index_; }
    bool operator==(const CongruenceClass &other) const;
};

namespace std {
template <>
// overload std::less for std::shared_ptr<CongruenceClass>, i.e. how to sort the congruence classes
struct less<std::shared_ptr<CongruenceClass>> {
    bool operator()(const std::shared_ptr<CongruenceClass> &a, const std::shared_ptr<CongruenceClass> &b) const {
        // nullptrs should never appear in partitions, so we just dereference it
        return *a < *b;
    }
};
} // namespace std

class GVN : public Pass {
  public:
    using partitions = std::set<std::shared_ptr<CongruenceClass>>;  // partition(分组) 是一个 CongruenceClass(等价类) 的容器
    GVN(Module *m, bool dump_json) : Pass(m), dump_json_(dump_json) {}
    // pass start
    void run() override;
    // init for pass metadata;
    void initPerFunction();

    // fill the following functions according to Pseudocode, **you might need to add more arguments**
    void detectEquivalences();
    partitions join(const partitions &P1, const partitions &P2);
    std::shared_ptr<CongruenceClass> intersect(std::shared_ptr<CongruenceClass>, std::shared_ptr<CongruenceClass>);
    partitions transferFunction(Instruction *x, Value *e, partitions pin);
    std::shared_ptr<GVNExpression::PhiExpression> valuePhiFunc(std::shared_ptr<GVNExpression::Expression>,
                                                               const partitions &);
    std::shared_ptr<GVNExpression::Expression> valueExpr(Instruction *instr);
    std::shared_ptr<GVNExpression::Expression> getVN(const partitions &pout,
                                                     std::shared_ptr<GVNExpression::Expression> ve);

    // replace cc members with leader
    void replace_cc_members();

    // note: be careful when to use copy constructor or clone
    partitions clone(const partitions &p);

    // create congruence class helper
    std::shared_ptr<CongruenceClass> createCongruenceClass(size_t index = 0) {
        return std::make_shared<CongruenceClass>(index);
    }

  private:
    bool dump_json_;
    std::uint64_t next_value_number_ = 1;
    Function *func_;
    std::map<BasicBlock *, partitions> pin_, pout_;
    std::unique_ptr<FuncInfo> func_info_;
    std::unique_ptr<GVNExpression::ConstFolder> folder_;
    std::unique_ptr<DeadCode> dce_;
};

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2);
