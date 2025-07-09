#include "WFCGenerator.h"
#include <iostream>

/**
 * @brief 将 Direction 枚举转换为字符串。
 * @param dir 要转换的方向枚举。
 * @return 对应方向的字符串表示。
 */
std::string directionToString(Direction dir) {
    switch (dir) {
    case TOP: return "TOP";
    case BOTTOM: return "BOTTOM";
    case LEFT: return "LEFT";
    case RIGHT: return "RIGHT";
    default: return "UNKNOWN";
    }
}

/**
 * @brief WFCGenerator 构造函数。
 * @param width 网格宽度。
 * @param height 网格高度。
 * @param modules 用于生成的所有模块的列表。
 */
WFCGenerator::WFCGenerator(int width, int height, const std::vector<Module>& modules)
    : width(width), height(height), allModules(modules),
    // 使用当前系统时间作为默认随机数种子，确保每次运行结果不同
    gen(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())) {
    // 调整网格大小以匹配指定的宽度和高度
    grid.resize(height, std::vector<Cell*>(width, nullptr));
    // 初始化网格，创建所有 Cell 对象
    initializeGrid();
}

/**
 * @brief 设置随机数生成器的种子。
 * @param seed 要使用的种子值。
 */
void WFCGenerator::setSeed(unsigned int seed) {
    gen.seed(seed); // 使用传入的种子重置随机数生成器
}

/**
 * @brief WFCGenerator 析构函数。
 * 负责清理动态分配的 Cell 对象，防止内存泄漏。
 */
WFCGenerator::~WFCGenerator() {
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            delete grid[i][j];
        }
    }
}

/**
 * @brief 设置特定模块的全局数量上限。
 * @param moduleId 要限制的模块ID。
 * @param limit 数量上限。
 */
void WFCGenerator::setGlobalModuleLimit(const std::string& moduleId, int limit) {
    globalModuleLimits[moduleId] = limit;
}

/**
 * @brief 获取对内部网格的常量引用。
 * @return 一个常量引用，指向存储 Cell 指针的二维向量。
 */
const std::vector<std::vector<Cell*>>& WFCGenerator::getGrid() const {
    return grid;
}

//  获取当前网格中各模块的计数映射。
const std::map<std::string, int>& WFCGenerator::getGlobalModuleCounts() const {
    return globalModuleCounts;
}

/**
 * @brief 在控制台打印当前网格的状态（用于调试）。
 * 已坍缩的单元格显示其模块ID，未坍缩的显示 '?'。
 */
void WFCGenerator::printGrid() const {
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            if (grid[i][j]->isCollapsed) {
                std::cout << grid[i][j]->chosenModuleId << "\t";
            }
            else {
                std::cout << "?\t";
            }
        }
        std::cout << std::endl;
    }
}

/**
 * @brief 初始化网格，为每个位置创建一个新的 Cell 对象。
 */
void WFCGenerator::initializeGrid() {
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            grid[i][j] = new Cell(j, i, allModules);
        }
    }
}

/**
 * @brief 查找并返回熵最低的未坍缩单元格。
 * 如果有多个熵最低的单元格，则从中随机选择一个。
 * @return 指向熵最低的单元格的指针，如果所有单元格都已坍缩，则返回 nullptr。
 */
Cell* WFCGenerator::getLowestEntropyCell() {
    Cell* lowestEntropyCell = nullptr;
    size_t minEntropy = -1; // 使用最大值初始化最小熵
    std::vector<Cell*> candidates; // 存储所有熵最低的候选单元格

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            Cell* cell = grid[i][j];
            if (!cell->isCollapsed && !cell->possibleModules.empty()) {
                size_t currentEntropy = cell->calculateEntropy();
                if (lowestEntropyCell == nullptr || currentEntropy < minEntropy) {
                    minEntropy = currentEntropy;
                    candidates.clear();
                    candidates.push_back(cell);
                }
                else if (currentEntropy == minEntropy) {
                    candidates.push_back(cell);
                }
            }
        }
    }

    if (candidates.empty()) {
        return nullptr; // 没有可选择的单元格
    }
    // 从候选者中随机选择一个
    std::uniform_int_distribution<size_t> distrib(0, candidates.size() - 1);
    return candidates[distrib(gen)];
}

/**
 * @brief 根据权重和全局限制，为单元格选择一个模块进行坍缩。
 * @param cell 要坍缩的单元格。
 * @param chosenModuleId [out] 用于存储选定模块ID的引用。
 * @return 如果成功选择一个模块，返回 true；否则返回 false。
 */
bool WFCGenerator::collapseCell(Cell* cell, std::string& chosenModuleId) {
    if (cell->possibleModules.empty()) {
        return false;
    }

    std::vector<std::string> weightedModules; // 存储符合条件的模块ID
    std::vector<double> weights;              // 存储对应的权重

    // 遍历所有可能的模块，筛选出符合全局限制的模块
    for (const auto& moduleId : cell->possibleModules) {
        const Module* modulePtr = nullptr;
        for (const auto& m : allModules) {
            if (m.id == moduleId) {
                modulePtr = &m;
                break;
            }
        }

        if (modulePtr) {
            // 如果模块有全局限制，并且当前计数已达到上限，则跳过
            if (globalModuleLimits.count(moduleId) && globalModuleCounts[moduleId] >= globalModuleLimits[moduleId]) {
                continue;
            }
            weightedModules.push_back(moduleId);
            weights.push_back(modulePtr->weight);
        }
    }

    if (weightedModules.empty()) {
        return false; // 没有可用的模块可选
    }

    // 使用离散分布根据权重随机选择一个模块
    std::discrete_distribution<> distrib(weights.begin(), weights.end());
    chosenModuleId = weightedModules[distrib(gen)];

    return true;
}

/**
 * @brief 从一个点开始，向外传播约束。
 * 当一个单元格的状态改变时，此函数会更新其邻居的可能模块列表。
 * @param startX 起始单元格的X坐标。
 * @param startY 起始单元格的Y坐标。
 * @return 如果传播没有导致矛盾（即没有单元格的可能模块变为空），返回 true。
 */
bool WFCGenerator::propagate(int startX, int startY) {
    std::vector<std::pair<int, int>> stack; // 使用栈来管理需要检查的单元格
    stack.push_back({ startX, startY });

    while (!stack.empty()) {
        std::pair<int, int> current = stack.back();
        stack.pop_back();
        int x = current.first;
        int y = current.second;

        Cell* currentCell = grid[y][x];
        // 获取当前单元格的可能模块集合
        std::set<std::string> possibleModulesInCurrentCell = currentCell->isCollapsed ?
            std::set<std::string>{currentCell->chosenModuleId} :
            currentCell->possibleModules;

        // 定义四个方向的偏移量
        int dx[] = { 0, 0, -1, 1 }; // TOP, BOTTOM, LEFT, RIGHT
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            Direction dir = static_cast<Direction>(i);

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                Cell* neighborCell = grid[ny][nx];
                if (neighborCell->isCollapsed) continue;

                std::set<std::string> modulesToRemove;
                // 检查邻居的每个可能模块
                for (const auto& possibleNeighborModuleId : neighborCell->possibleModules) {
                    const Module* possibleNeighborModule = nullptr;
                    for (const auto& m : allModules) if (m.id == possibleNeighborModuleId) possibleNeighborModule = &m;

                    if (!possibleNeighborModule) continue;

                    // 检查当前单元格是否有任何模块可以支持这个邻居模块
                    bool isSupported = false;
                    for (const auto& possibleCurrentModuleId : possibleModulesInCurrentCell) {
                        const Module* possibleCurrentModule = nullptr;
                        for (const auto& m : allModules) if (m.id == possibleCurrentModuleId) possibleCurrentModule = &m;

                        if (possibleCurrentModule && possibleCurrentModule->isCompatible(dir, *possibleNeighborModule)) {
                            isSupported = true;
                            break;
                        }
                    }

                    // 如果没有任何模块支持，则将该邻居模块标记为待移除
                    if (!isSupported) {
                        modulesToRemove.insert(possibleNeighborModuleId);
                    }
                }

                // 如果有需要移除的模块
                if (!modulesToRemove.empty()) {
                    bool changed = false;
                    for (const auto& moduleId : modulesToRemove) {
                        if (neighborCell->removePossibleModule(moduleId)) {
                            changed = true;
                        }
                    }
                    // 如果邻居的状态发生了变化，则将其加入栈中以便进一步传播
                    if (changed) {
                        if (neighborCell->possibleModules.empty()) {
                            return false; // 产生矛盾，传播失败
                        }
                        stack.push_back({ nx, ny });
                    }
                }
            }
        }
    }
    return true; // 传播成功
}

/**
 * @brief 在坍缩一个单元格之前，保存当前的状态快照。
 * @param cellToCollapse 即将坍缩的单元格。
 * @param chosenModuleId 为该单元格选择的模块ID。
 */
void WFCGenerator::saveState(Cell* cellToCollapse, const std::string& chosenModuleId) {
    std::map<std::pair<int, int>, std::set<std::string>> currentGridState;
    // 记录整个网格中每个单元格的可能模块
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            currentGridState[{j, i}] = grid[i][j]->possibleModules;
        }
    }
    // 将快照压入栈中
    stateStack.push({ cellToCollapse->x, cellToCollapse->y,
                     cellToCollapse->possibleModules,
                     chosenModuleId,
                     currentGridState,
                     globalModuleCounts });
}

/**
 * @brief 执行回溯。
 * 当遇到矛盾时，恢复到上一个保存的状态，并从失败的选项中移除该选项。
 * @return 如果回溯成功并且传播没有立即导致新的矛盾，返回 true。
 */
bool WFCGenerator::backtrack() {
    if (stateStack.empty()) {
        return false; // 没有可回溯的状态
    }

    StateSnapshot lastState = stateStack.top();
    stateStack.pop();

    // 恢复整个网格的状态
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            grid[i][j]->isCollapsed = false;
            grid[i][j]->chosenModuleId = "";
            if (lastState.gridStateSnapshot.count({ j, i })) {
                grid[i][j]->possibleModules = lastState.gridStateSnapshot.at({ j, i });
            }
        }
    }

    // 恢复全局模块计数
    globalModuleCounts = lastState.globalModuleCountsSnapshot;

    // 从导致失败的单元格的可能模块中，移除那个失败的选项
    Cell* failedCell = grid[lastState.cellY][lastState.cellX];
    failedCell->removePossibleModule(lastState.attemptedModuleId);

    // 如果移除后该单元格没有其他可能性，则需要进一步回溯
    if (failedCell->possibleModules.empty()) {
        return backtrack();
    }

    std::cout << "Backtracking from cell (" << failedCell->x << ", " << failedCell->y
        << "). Removed module " << lastState.attemptedModuleId << " from possibilities." << std::endl;

    // 从失败的单元格开始重新传播约束
    return propagate(failedCell->x, failedCell->y);
}

/**
 * @brief 主生成循环。
 * 持续选择熵最低的单元格进行坍缩和传播，直到所有单元格都坍缩或无法找到解。
 * @return 如果成功生成完整网格，返回 true。
 */
bool WFCGenerator::generate() {
    int collapsedCount = 0;
    int totalCells = width * height;

    while (collapsedCount < totalCells)
    {
        // 1. 选择熵最低的单元格
        Cell* targetCell = getLowestEntropyCell();
        if (!targetCell) {
            if (collapsedCount == totalCells) break; // 所有单元格都已坍缩，成功
            std::cout << "Error: No valid cell to collapse, but not all cells are collapsed." << std::endl;
            if (!backtrack()) { // 无法选择单元格，尝试回溯
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            continue;
        }

        // 如果选中的单元格已经没有可能模块，说明出现矛盾
        if (targetCell->possibleModules.empty()) {
            std::cout << "Contradiction found at (" << targetCell->x << ", " << targetCell->y << "). Attempting to backtrack..." << std::endl;
            if (!backtrack()) {
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            continue;
        }

        // 2. 坍缩单元格
        std::string chosenModuleId;
        if (!collapseCell(targetCell, chosenModuleId)) {
            std::cout << "Collapse failed at (" << targetCell->x << ", " << targetCell->y << "), likely due to global constraints. Backtracking..." << std::endl;
            if (!backtrack()) {
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            continue;
        }

        // 保存当前状态以便可能的回溯
        saveState(targetCell, chosenModuleId);

        // 更新单元格状态
        targetCell->isCollapsed = true;
        targetCell->chosenModuleId = chosenModuleId;
        targetCell->possibleModules = { chosenModuleId };
        for (const auto& m : allModules) {
            if (m.id == chosenModuleId) {
                targetCell->module = &m;
                break;
            }
        }
        globalModuleCounts[chosenModuleId]++;

        // 重新计算已坍缩的单元格数量
        collapsedCount = 0;
        for (int i = 0; i < height; ++i) for (int j = 0; j < width; ++j) if (grid[i][j]->isCollapsed) collapsedCount++;

        // 3. 传播约束
        if (!propagate(targetCell->x, targetCell->y)) {
            std::cout << "Propagation led to a contradiction. Backtracking..." << std::endl;
            if (!backtrack()) {
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            // 回溯后需要重新计算已坍缩的单元格数量
            collapsedCount = 0;
            for (int i = 0; i < height; ++i) for (int j = 0; j < width; ++j) if (grid[i][j]->isCollapsed) collapsedCount++;
        }
    }

    // 检查是否所有单元格都已成功坍缩
    if (collapsedCount == width * height) {
        std::cout << "WFC generation successful!" << std::endl;
        return true;
    }
    else {
        std::cout << "WFC generation failed." << std::endl;
        return false;
    }
}