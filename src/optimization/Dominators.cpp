#include "Dominators.h"

#include <algorithm>
#include <string>


/*
处理模块中的每个函数，计算出其中每个基本块的支配者和支配边界
*/
void Dominators::run() {
    for (auto &f1 : m_->get_functions()) {
        auto f = &f1;
        if (f->get_basic_blocks().size() == 0)
            continue;
        for (auto &bb1 : f->get_basic_blocks()) {
            auto bb = &bb1;
            doms_.insert({bb, {}});
            idom_.insert({bb, {}});
            dom_frontier_.insert({bb, {}});
            dom_tree_succ_blocks_.insert({bb, {}});
        }

        create_reverse_post_order(f);   // 创建一个给定函数的基本块的逆后序排列
        create_idom(f);                 // 计算出每个基本块的直接支配者
        create_dominance_frontier(f);   // 计算出每个基本块的支配边界
        create_dom_tree_succ(f);        // 创建一个给定函数的基本块的支配者树
        // for debug
        // print_idom(f);
        // print_dominance_frontier(f);
    }
}

/*
该函数计算基本块的支配节点的函数
*/
void Dominators::create_doms(Function *f) {
    // init
    for (auto &bb1 : f->get_basic_blocks()) {   // 初始化阶段，对于每个基本块，将其添加为它自己的支配节点
        auto bb = &bb1;
        add_dom(bb, bb);
    }
    // iterate
    bool changed = true;
    std::vector<BasicBlock *> ret(f->get_num_basic_blocks());
    std::vector<BasicBlock *> pre(f->get_num_basic_blocks());
    while (changed) {   // 在迭代阶段，对于每个基本块，通过计算它的前驱块的支配节点的交集，来更新它的支配节点, 该过程持续到支配节点不再发生变化为止
        changed = false;
        for (auto &bb1 : f->get_basic_blocks()) {
            auto bb = &bb1;
            auto &bbs = bb->get_pre_basic_blocks();
            auto &first = get_doms((*bbs.begin()));
            pre.insert(pre.begin(), first.begin(), first.end());    // vector.insert(position, begin, end) 在指定位置插入从 begin 到 end 的元素
            pre.resize(first.size());       // 每次迭代更改前驱块的支配结点集的大小
            ret.resize(f->get_num_basic_blocks());
            for (auto iter = ++bbs.begin(); iter != bbs.end(); ++iter) {    // 遍历每一个前驱块
                auto &now = get_doms((*iter));
                auto it = std::set_intersection(pre.begin(), pre.end(), now.begin(), now.end(), ret.begin());   // 求前驱快支配者的交集
                ret.resize(it - ret.begin());
                pre.resize(ret.size());
                pre.insert(pre.begin(), ret.begin(), ret.end());
            }
            std::set<BasicBlock *> doms;
            doms.insert(bb);
            doms.insert(pre.begin(), pre.end());
            if (get_doms(bb) != doms) {
                set_doms(bb, doms);
                changed = true;
            }
        }
    }
}

/*
翻转后序遍历算法被用作基础块支配信息计算的第一步，后面的步骤会使用到该遍历序列
*/
void Dominators::create_reverse_post_order(Function *f) {
    reverse_post_order_.clear();
    post_order_id_.clear();
    std::set<BasicBlock *> visited;
    post_order_visit(f->get_entry_block(), visited);
    reverse_post_order_.reverse();
}

/*
这段代码实现了一个后序遍历函数 post_order_visit()。它接收两个参数：一个基本块 bb，和一个用来记录访问过的基本块集合 visited。

首先，将当前基本块 bb 插入到已访问过的基本块集合 visited 中，表示这个基本块已经被访问过了。然后，遍历 bb 的所有后继基本块。如果这个后继基本块还没有被访问过，那么就对它进行递归调用。

最后，将当前基本块 bb 插入到后序遍历序列中，并记录它在后序遍历序列中的下标。这个后序遍历序列会用于计算基本块的支配者。

该函数还有一个变量 reverse_post_order_，它用来记录基本块的后序遍历序列。该函数还有一个变量 post_order_id_，它用来记录基本块在后序遍历序列中的下标。这两个变量在整个算法中都会被用到。
*/
void Dominators::post_order_visit(BasicBlock *bb, std::set<BasicBlock *> &visited) {
    visited.insert(bb);
    for (auto b : bb->get_succ_basic_blocks()) {
        if (visited.find(b) == visited.end())
            post_order_visit(b, visited);
    }
    post_order_id_[bb] = reverse_post_order_.size();
    reverse_post_order_.push_back(bb);
}


/*
这段代码实现了求一个函数的支配节点（idom）的算法。

首先，它会初始化所有基本块的 idom 为 nullptr，并将入口节点的 idom 设为入口节点本身。

然后，它会进入一个循环，在每次循环中，它会以 reverse_post_order_ 为顺序遍历所有基本块。对于每个基本块，它会找到它的一个前驱节点，该节点的 idom 已经被求出。

然后，它会把该前驱节点的 idom 设为当前基本块的 idom，并遍历所有前驱节点，寻找最终的 idom。如果在遍历完所有前驱节点之后，当前基本块的 idom 发生了变化，那么就将 changed 设为 true，并继续进行下一轮循环。

循环结束后，所有基本块的 idom 都已经被求出。
*/
void Dominators::create_idom(Function *f) {
    // init
    for (auto &bb : f->get_basic_blocks())
        set_idom(&bb, nullptr);
    auto root = f->get_entry_block();
    set_idom(root, root);

    // iterate
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto bb : this->reverse_post_order_) {
            if (bb == root) {
                continue;
            }

            // find one pred which has idom
            BasicBlock *pred = nullptr;
            for (auto p : bb->get_pre_basic_blocks()) {
                if (get_idom(p)) {
                    pred = p;
                    break;
                }
            }
            assert(pred);

            BasicBlock *new_idom = pred;
            for (auto p : bb->get_pre_basic_blocks()) {
                if (p == pred)
                    continue;
                if (get_idom(p)) {
                    new_idom = intersect(p, new_idom);
                }
            }
            if (get_idom(bb) != new_idom) {
                set_idom(bb, new_idom);
                changed = true;
            }
        }
    }
}

/*
求出两个基本块的支配节点的交集。

它首先会不断地跳到每个基本块的支配节点，直到两个基本块的支配节点相同为止。这样就可以求出两个基本块的支配节点的交集了。
*/
// find closest parent of b1 and b2
BasicBlock *Dominators::intersect(BasicBlock *b1, BasicBlock *b2) {
    while (b1 != b2) {
        while (post_order_id_[b1] < post_order_id_[b2]) {
            assert(get_idom(b1));
            b1 = get_idom(b1);
        }
        while (post_order_id_[b2] < post_order_id_[b1]) {
            assert(get_idom(b2));
            b2 = get_idom(b2);
        }
    }
    return b1;
}

/*
求支配边界, 首先，对于每个基本块，如果它的前驱基本块的个数不小于2，那么对于每个前驱基本块，执行以下操作：

从该前驱基本块开始，一直跳它的 idom，直到跳到当前基本块的 idom 为止。在每一步跳 idom 的时候，都将当前基本块作为 runner 加入到 runner 的支配边界中。

这样做的原因是，对于一个基本块 bb，它的支配边界是由它的所有不支配它的前驱基本块所构成的。对于某个不支配 bb 的前驱基本块 runner，它必定存在一条路径，从 runner 开始，一直跳它的 idom，到达 bb
*/
void Dominators::create_dominance_frontier(Function *f) {
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        if (bb->get_pre_basic_blocks().size() >= 2) {
            for (auto p : bb->get_pre_basic_blocks()) {
                auto runner = p;
                while (runner != get_idom(bb)) {
                    add_dominance_frontier(runner, bb);
                    runner = get_idom(runner);
                }
            }
        }
    }
}

/*
创建支配树的后继节点。首先对于每个基本块，它的第一步是找到它的祖先块，然后把它作为祖先块的后继节点。

如果这个块没有祖先块（例如，入口块），那么这个块就不会成为任何块的后继块
*/
void Dominators::create_dom_tree_succ(Function *f) {
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        auto idom = get_idom(bb);
        // e.g, entry bb
        if (idom != bb) {
            add_dom_tree_succ_block(idom, bb);
        }
    }
}

/*
该函数用于在标准输出设备（通常是屏幕）中输出某个函数的"立即支配"（immediate dominance）信息。

在控制流图（CFG）中，一个基本块 B1 的立即支配块（immediate dominator，简称 IDOM）是定义为：对于 B1 的任意前驱块 B2，B1 的 IDOM 是唯一一个同时支配 B1 和 B2 的基本块。

这段代码的主要作用是为每个基本块生成一个编号，然后按照如下格式输出每个基本块的 IDOM：bb1: bb2   (其中 bb1 是基本块的编号，bb2 是它的 IDOM 的编号)

假设有一个函数 f 包含了三个基本块：bb1、bb2 和 bb3，bb1 是进入块（entry block），bb2 是 bb1 的唯一后继块，bb3 是 bb2 的唯一后继块，那么 f 的 IDOM 信息应该是这样的：
bb1: bb1
bb2: bb1
bb3: bb2
*/
void Dominators::print_idom(Function *f) {
    int counter = 0;
    std::map<BasicBlock *, std::string> bb_id;
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        if (bb->get_name().empty())
            bb_id[bb] = "bb" + std::to_string(counter);
        else
            bb_id[bb] = bb->get_name();
        counter++;
    }
    printf("Immediate dominance of function %s:\n", f->get_name().c_str());
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        std::string output;
        output = bb_id[bb] + ": ";
        if (get_idom(bb)) {
            output += bb_id[get_idom(bb)];
        } else {
            output += "null";
        }
        printf("%s\n", output.c_str());
    }
}

/*
输出一个函数的支配边界信息。

它首先为每个基本块分配一个唯一的 ID，然后遍历函数的所有基本块，输出每个基本块的支配边界。

如果一个基本块没有支配边界，则输出 null，否则输出基本块的 ID
*/
void Dominators::print_dominance_frontier(Function *f) {
    int counter = 0;
    std::map<BasicBlock *, std::string> bb_id;
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        if (bb->get_name().empty())
            bb_id[bb] = "bb" + std::to_string(counter);
        else
            bb_id[bb] = bb->get_name();
        counter++;
    }
    printf("Dominance Frontier of function %s:\n", f->get_name().c_str());
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        std::string output;
        output = bb_id[bb] + ": ";
        if (get_dominance_frontier(bb).empty()) {
            output += "null";
        } else {
            bool first = true;
            for (auto df : get_dominance_frontier(bb)) {
                if (first) {
                    first = false;
                } else {
                    output += ", ";
                }
                output += bb_id[df];
            }
        }
        printf("%s\n", output.c_str());
    }
}
