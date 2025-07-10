#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>
#include <stack>
#include <SFML/System/Vector2.hpp>

/**
 * @brief 定义四个基本方向。
 * COUNT 用于获取方向的总数。
 */
enum Direction {
    TOP,    // 上
    BOTTOM, // 下
    LEFT,   // 左
    RIGHT,  // 右
    COUNT   // 方向总数
};

// 辅助函数声明
std::string directionToString(Direction dir);

/**
 * @class Module
 * @brief 代表一个生成单元（或瓦片）。
 * 每个模块都有其独特的ID、权重、邻接规则和在瓦片集上的位置。
 */
class Module {
public:
    std::string id;                                     // 模块的唯一标识符
    double weight;                                      // 模块在选择时的权重，影响其出现频率
    std::map<Direction, std::set<std::string>> adjacencyRules; // 邻接规则，定义了在各个方向上可以与哪些模块ID相邻
    sf::Vector2i tileIndex;                             // 模块在瓦片集（tileset）纹理上的索引坐标

    /**
     * @brief Module 构造函数。
     * @param id 模块ID。
     * @param weight 模块权重。
     */
    Module(std::string id, double weight) : id(id), weight(weight), tileIndex(0, 0) {}

    /**
     * @brief 检查当前模块是否可以在指定方向上与另一个模块兼容。
     * 兼容性是双向的：A的上方必须能接B，同时B的下方也必须能接A。
     * @param dir 相对于当前模块的方向。
     * @param otherModule 要检查兼容性的另一个模块。
     * @return 如果兼容则返回 true，否则返回 false。
     */
    bool isCompatible(Direction dir, const Module& otherModule) const {
        Direction oppositeDir;
        if (dir == TOP) oppositeDir = BOTTOM;
        else if (dir == BOTTOM) oppositeDir = TOP;
        else if (dir == LEFT) oppositeDir = RIGHT;
        else oppositeDir = LEFT;

        // 检查双方是否都定义了对应方向的规则
        if (adjacencyRules.count(dir) == 0 || otherModule.adjacencyRules.count(oppositeDir) == 0) {
            return false;
        }

        // 检查规则是否相互匹配
        return adjacencyRules.at(dir).count(otherModule.id) &&
            otherModule.adjacencyRules.at(oppositeDir).count(this->id);
    }
};

/**
 * @class Cell
 * @brief 代表输出网格中的一个单元格。
 * 每个单元格维护一组可能的模块，直到它被“坍缩”为一个确定的模块。
 */
class Cell {
public:
    int x, y;                               // 单元格在网格中的坐标
    bool isCollapsed;                       // 标记单元格是否已坍缩
    std::set<std::string> possibleModules;  // 存储当前单元格所有可能的模块ID集合
    std::string chosenModuleId;             // 坍缩后选定的模块ID
    const Module* module = nullptr;         // 指向坍缩后选定的模块对象的指针


    /**
     * @brief Cell 构造函数。
     * @param x X坐标。
     * @param y Y坐标。
     * @param allModules 所有可用模块的列表，用于初始化 possibleModules。
     */
    Cell(int x, int y, const std::vector<Module>& allModules) : x(x), y(y), isCollapsed(false) {
        for (const auto& module : allModules) {
            possibleModules.insert(module.id);
        }
    }

    /**
     * @brief 计算单元格的熵。
     * 在WFC中，熵通常是可能模块的数量。熵越低，不确定性越小。
     * @return 可能模块的数量。
     */
    size_t calculateEntropy() const {
        return possibleModules.size();
    }

    /**
     * @brief 从可能模块集合中移除一个模块ID。
     * 这是WFC传播阶段的核心操作。
     * @param moduleId 要移除的模块ID。
     * @return 如果成功移除了模块，则返回 true。
     */
    bool removePossibleModule(const std::string& moduleId) {
        if (possibleModules.count(moduleId)) {
            possibleModules.erase(moduleId);
            return true;
        }
        return false;
    }

    /**
     * @brief 重置单元格状态。
     * 用于回溯时将单元格恢复到之前的某个状态。
     * @param initialPossibleModules 要恢复到的可能模块集合。
     */
    void reset(const std::set<std::string>& initialPossibleModules) {
        isCollapsed = false;
        chosenModuleId = "";
        possibleModules = initialPossibleModules;
    }
};

/**
 * @struct StateSnapshot
 * @brief 用于在回溯时保存和恢复算法的关键状态。
 * 当选择一个模块进行坍缩时，会创建一个快照。如果后续生成失败，则使用此快照进行回溯。
 */
struct StateSnapshot {
    int cellX, cellY;                                           // 发生坍缩的单元格坐标
    std::set<std::string> initialPossibleModules;               // 坍缩前该单元格的可能模块
    std::string attemptedModuleId;                              // 尝试坍缩成的模块ID
    std::map<std::pair<int, int>, std::set<std::string>> gridStateSnapshot; // 整个网格在坍缩前的状态
    std::map<std::string, int> globalModuleCountsSnapshot;      // 全局模块计数在坍缩前的状态

    /**
     * @brief StateSnapshot 构造函数。
     */
    StateSnapshot(int x, int y, const std::set<std::string>& possible,
        const std::string& attempted,
        const std::map<std::pair<int, int>, std::set<std::string>>& currentGridState,
        const std::map<std::string, int>& currentGlobalModuleCounts)
        : cellX(x), cellY(y), initialPossibleModules(possible), attemptedModuleId(attempted),
        gridStateSnapshot(currentGridState), globalModuleCountsSnapshot(currentGlobalModuleCounts) {
    }
};

/**
 * @class WFCGenerator
 * @brief 波函数坍缩（WFC）算法的核心实现类。
 * 管理网格、模块、生成过程、回溯等。
 */
class WFCGenerator {
public:
    /**
     * @brief WFCGenerator 构造函数。
     * @param width 生成网格的宽度。
     * @param height 生成网格的高度。
     * @param modules 用于生成的所有模块的列表。
     */
    WFCGenerator(int width, int height, const std::vector<Module>& modules);

    /**
     * @brief WFCGenerator 析构函数。
     * 负责释放网格中动态分配的 Cell 对象。
     */
    ~WFCGenerator();

    /**
     * @brief 设置特定模块在整个网格中的数量上限。
     * @param moduleId 要限制的模块ID。
     * @param limit 数量上限。
     */
    void setGlobalModuleLimit(const std::string& moduleId, int limit);

    /**
     * @brief 启动WFC生成过程。
     * @return 如果成功生成完整网格则返回 true，否则返回 false。
     */
    bool generate(bool useRelaxation = false);

    /**
     * @brief 获取生成的网格数据。
     * @return 一个常量引用，指向存储 Cell 指针的二维向量。
     */
    const std::vector<std::vector<Cell*>>& getGrid() const;

    const std::map<std::string, int>& getGlobalModuleCounts() const;

    /**
     * @brief 在控制台打印生成的网格（用于调试）。
     */
    void printGrid() const;

    /**
     * @brief 设置用于随机数生成的种子。
     * @param seed 随机数种子。
     */
    void setSeed(unsigned int seed);

    void removePossibility(int x, int y, const std::string& moduleId);

private:
    int width, height;                                  // 网格尺寸
    std::vector<std::vector<Cell*>> grid;               // 存储网格单元格的二维向量
    std::vector<Module> allModules;                     // 所有可用模块的副本
    std::map<std::string, int> globalModuleCounts;      // 当前网格中各模块的计数
    std::map<std::string, int> globalModuleLimits;      // 各模块的全局数量上限
    std::stack<StateSnapshot> stateStack;               // 用于回溯的状态快照栈
    std::mt19937 gen;                                   // 随机数生成器

    // 私有辅助函数
    void initializeGrid();                              // 初始化网格，创建所有 Cell 对象
    Cell* getLowestEntropyCell();                       // 查找并返回熵最低（最不确定）的未坍缩单元格
    bool collapseCell(Cell* cell, std::string& chosenModuleId); // 坍缩一个单元格，为其选择一个确定的模块
    bool propagate(int startX, int startY);             // 从一个点开始，向外传播约束，更新邻居单元格的可能模块
    void saveState(Cell* cellToCollapse, const std::string& chosenModuleId); // 保存当前状态到快照栈
    bool backtrack();                                   // 执行回溯，恢复到上一个状态并尝试其他选择
};