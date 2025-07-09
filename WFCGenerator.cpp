#include "WFCGenerator.h"
#include <iostream>

/**
 * @brief �� Direction ö��ת��Ϊ�ַ�����
 * @param dir Ҫת���ķ���ö�١�
 * @return ��Ӧ������ַ�����ʾ��
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
 * @brief WFCGenerator ���캯����
 * @param width �����ȡ�
 * @param height ����߶ȡ�
 * @param modules �������ɵ�����ģ����б�
 */
WFCGenerator::WFCGenerator(int width, int height, const std::vector<Module>& modules)
    : width(width), height(height), allModules(modules),
    // ʹ�õ�ǰϵͳʱ����ΪĬ����������ӣ�ȷ��ÿ�����н����ͬ
    gen(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())) {
    // ���������С��ƥ��ָ���Ŀ�Ⱥ͸߶�
    grid.resize(height, std::vector<Cell*>(width, nullptr));
    // ��ʼ�����񣬴������� Cell ����
    initializeGrid();
}

/**
 * @brief ��������������������ӡ�
 * @param seed Ҫʹ�õ�����ֵ��
 */
void WFCGenerator::setSeed(unsigned int seed) {
    gen.seed(seed); // ʹ�ô�����������������������
}

/**
 * @brief WFCGenerator ����������
 * ��������̬����� Cell ���󣬷�ֹ�ڴ�й©��
 */
WFCGenerator::~WFCGenerator() {
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            delete grid[i][j];
        }
    }
}

/**
 * @brief �����ض�ģ���ȫ���������ޡ�
 * @param moduleId Ҫ���Ƶ�ģ��ID��
 * @param limit �������ޡ�
 */
void WFCGenerator::setGlobalModuleLimit(const std::string& moduleId, int limit) {
    globalModuleLimits[moduleId] = limit;
}

/**
 * @brief ��ȡ���ڲ�����ĳ������á�
 * @return һ���������ã�ָ��洢 Cell ָ��Ķ�ά������
 */
const std::vector<std::vector<Cell*>>& WFCGenerator::getGrid() const {
    return grid;
}

//  ��ȡ��ǰ�����и�ģ��ļ���ӳ�䡣
const std::map<std::string, int>& WFCGenerator::getGlobalModuleCounts() const {
    return globalModuleCounts;
}

/**
 * @brief �ڿ���̨��ӡ��ǰ�����״̬�����ڵ��ԣ���
 * ��̮���ĵ�Ԫ����ʾ��ģ��ID��δ̮������ʾ '?'��
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
 * @brief ��ʼ������Ϊÿ��λ�ô���һ���µ� Cell ����
 */
void WFCGenerator::initializeGrid() {
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            grid[i][j] = new Cell(j, i, allModules);
        }
    }
}

/**
 * @brief ���Ҳ���������͵�δ̮����Ԫ��
 * ����ж������͵ĵ�Ԫ����������ѡ��һ����
 * @return ָ������͵ĵ�Ԫ���ָ�룬������е�Ԫ����̮�����򷵻� nullptr��
 */
Cell* WFCGenerator::getLowestEntropyCell() {
    Cell* lowestEntropyCell = nullptr;
    size_t minEntropy = -1; // ʹ�����ֵ��ʼ����С��
    std::vector<Cell*> candidates; // �洢��������͵ĺ�ѡ��Ԫ��

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
        return nullptr; // û�п�ѡ��ĵ�Ԫ��
    }
    // �Ӻ�ѡ�������ѡ��һ��
    std::uniform_int_distribution<size_t> distrib(0, candidates.size() - 1);
    return candidates[distrib(gen)];
}

/**
 * @brief ����Ȩ�غ�ȫ�����ƣ�Ϊ��Ԫ��ѡ��һ��ģ�����̮����
 * @param cell Ҫ̮���ĵ�Ԫ��
 * @param chosenModuleId [out] ���ڴ洢ѡ��ģ��ID�����á�
 * @return ����ɹ�ѡ��һ��ģ�飬���� true�����򷵻� false��
 */
bool WFCGenerator::collapseCell(Cell* cell, std::string& chosenModuleId) {
    if (cell->possibleModules.empty()) {
        return false;
    }

    std::vector<std::string> weightedModules; // �洢����������ģ��ID
    std::vector<double> weights;              // �洢��Ӧ��Ȩ��

    // �������п��ܵ�ģ�飬ɸѡ������ȫ�����Ƶ�ģ��
    for (const auto& moduleId : cell->possibleModules) {
        const Module* modulePtr = nullptr;
        for (const auto& m : allModules) {
            if (m.id == moduleId) {
                modulePtr = &m;
                break;
            }
        }

        if (modulePtr) {
            // ���ģ����ȫ�����ƣ����ҵ�ǰ�����Ѵﵽ���ޣ�������
            if (globalModuleLimits.count(moduleId) && globalModuleCounts[moduleId] >= globalModuleLimits[moduleId]) {
                continue;
            }
            weightedModules.push_back(moduleId);
            weights.push_back(modulePtr->weight);
        }
    }

    if (weightedModules.empty()) {
        return false; // û�п��õ�ģ���ѡ
    }

    // ʹ����ɢ�ֲ�����Ȩ�����ѡ��һ��ģ��
    std::discrete_distribution<> distrib(weights.begin(), weights.end());
    chosenModuleId = weightedModules[distrib(gen)];

    return true;
}

/**
 * @brief ��һ���㿪ʼ�����⴫��Լ����
 * ��һ����Ԫ���״̬�ı�ʱ���˺�����������ھӵĿ���ģ���б�
 * @param startX ��ʼ��Ԫ���X���ꡣ
 * @param startY ��ʼ��Ԫ���Y���ꡣ
 * @return �������û�е���ì�ܣ���û�е�Ԫ��Ŀ���ģ���Ϊ�գ������� true��
 */
bool WFCGenerator::propagate(int startX, int startY) {
    std::vector<std::pair<int, int>> stack; // ʹ��ջ��������Ҫ���ĵ�Ԫ��
    stack.push_back({ startX, startY });

    while (!stack.empty()) {
        std::pair<int, int> current = stack.back();
        stack.pop_back();
        int x = current.first;
        int y = current.second;

        Cell* currentCell = grid[y][x];
        // ��ȡ��ǰ��Ԫ��Ŀ���ģ�鼯��
        std::set<std::string> possibleModulesInCurrentCell = currentCell->isCollapsed ?
            std::set<std::string>{currentCell->chosenModuleId} :
            currentCell->possibleModules;

        // �����ĸ������ƫ����
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
                // ����ھӵ�ÿ������ģ��
                for (const auto& possibleNeighborModuleId : neighborCell->possibleModules) {
                    const Module* possibleNeighborModule = nullptr;
                    for (const auto& m : allModules) if (m.id == possibleNeighborModuleId) possibleNeighborModule = &m;

                    if (!possibleNeighborModule) continue;

                    // ��鵱ǰ��Ԫ���Ƿ����κ�ģ�����֧������ھ�ģ��
                    bool isSupported = false;
                    for (const auto& possibleCurrentModuleId : possibleModulesInCurrentCell) {
                        const Module* possibleCurrentModule = nullptr;
                        for (const auto& m : allModules) if (m.id == possibleCurrentModuleId) possibleCurrentModule = &m;

                        if (possibleCurrentModule && possibleCurrentModule->isCompatible(dir, *possibleNeighborModule)) {
                            isSupported = true;
                            break;
                        }
                    }

                    // ���û���κ�ģ��֧�֣��򽫸��ھ�ģ����Ϊ���Ƴ�
                    if (!isSupported) {
                        modulesToRemove.insert(possibleNeighborModuleId);
                    }
                }

                // �������Ҫ�Ƴ���ģ��
                if (!modulesToRemove.empty()) {
                    bool changed = false;
                    for (const auto& moduleId : modulesToRemove) {
                        if (neighborCell->removePossibleModule(moduleId)) {
                            changed = true;
                        }
                    }
                    // ����ھӵ�״̬�����˱仯���������ջ���Ա��һ������
                    if (changed) {
                        if (neighborCell->possibleModules.empty()) {
                            return false; // ����ì�ܣ�����ʧ��
                        }
                        stack.push_back({ nx, ny });
                    }
                }
            }
        }
    }
    return true; // �����ɹ�
}

/**
 * @brief ��̮��һ����Ԫ��֮ǰ�����浱ǰ��״̬���ա�
 * @param cellToCollapse ����̮���ĵ�Ԫ��
 * @param chosenModuleId Ϊ�õ�Ԫ��ѡ���ģ��ID��
 */
void WFCGenerator::saveState(Cell* cellToCollapse, const std::string& chosenModuleId) {
    std::map<std::pair<int, int>, std::set<std::string>> currentGridState;
    // ��¼����������ÿ����Ԫ��Ŀ���ģ��
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            currentGridState[{j, i}] = grid[i][j]->possibleModules;
        }
    }
    // ������ѹ��ջ��
    stateStack.push({ cellToCollapse->x, cellToCollapse->y,
                     cellToCollapse->possibleModules,
                     chosenModuleId,
                     currentGridState,
                     globalModuleCounts });
}

/**
 * @brief ִ�л��ݡ�
 * ������ì��ʱ���ָ�����һ�������״̬������ʧ�ܵ�ѡ�����Ƴ���ѡ�
 * @return ������ݳɹ����Ҵ���û�����������µ�ì�ܣ����� true��
 */
bool WFCGenerator::backtrack() {
    if (stateStack.empty()) {
        return false; // û�пɻ��ݵ�״̬
    }

    StateSnapshot lastState = stateStack.top();
    stateStack.pop();

    // �ָ����������״̬
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            grid[i][j]->isCollapsed = false;
            grid[i][j]->chosenModuleId = "";
            if (lastState.gridStateSnapshot.count({ j, i })) {
                grid[i][j]->possibleModules = lastState.gridStateSnapshot.at({ j, i });
            }
        }
    }

    // �ָ�ȫ��ģ�����
    globalModuleCounts = lastState.globalModuleCountsSnapshot;

    // �ӵ���ʧ�ܵĵ�Ԫ��Ŀ���ģ���У��Ƴ��Ǹ�ʧ�ܵ�ѡ��
    Cell* failedCell = grid[lastState.cellY][lastState.cellX];
    failedCell->removePossibleModule(lastState.attemptedModuleId);

    // ����Ƴ���õ�Ԫ��û�����������ԣ�����Ҫ��һ������
    if (failedCell->possibleModules.empty()) {
        return backtrack();
    }

    std::cout << "Backtracking from cell (" << failedCell->x << ", " << failedCell->y
        << "). Removed module " << lastState.attemptedModuleId << " from possibilities." << std::endl;

    // ��ʧ�ܵĵ�Ԫ��ʼ���´���Լ��
    return propagate(failedCell->x, failedCell->y);
}

/**
 * @brief ������ѭ����
 * ����ѡ������͵ĵ�Ԫ�����̮���ʹ�����ֱ�����е�Ԫ��̮�����޷��ҵ��⡣
 * @return ����ɹ������������񣬷��� true��
 */
bool WFCGenerator::generate() {
    int collapsedCount = 0;
    int totalCells = width * height;

    while (collapsedCount < totalCells)
    {
        // 1. ѡ������͵ĵ�Ԫ��
        Cell* targetCell = getLowestEntropyCell();
        if (!targetCell) {
            if (collapsedCount == totalCells) break; // ���е�Ԫ����̮�����ɹ�
            std::cout << "Error: No valid cell to collapse, but not all cells are collapsed." << std::endl;
            if (!backtrack()) { // �޷�ѡ��Ԫ�񣬳��Ի���
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            continue;
        }

        // ���ѡ�еĵ�Ԫ���Ѿ�û�п���ģ�飬˵������ì��
        if (targetCell->possibleModules.empty()) {
            std::cout << "Contradiction found at (" << targetCell->x << ", " << targetCell->y << "). Attempting to backtrack..." << std::endl;
            if (!backtrack()) {
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            continue;
        }

        // 2. ̮����Ԫ��
        std::string chosenModuleId;
        if (!collapseCell(targetCell, chosenModuleId)) {
            std::cout << "Collapse failed at (" << targetCell->x << ", " << targetCell->y << "), likely due to global constraints. Backtracking..." << std::endl;
            if (!backtrack()) {
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            continue;
        }

        // ���浱ǰ״̬�Ա���ܵĻ���
        saveState(targetCell, chosenModuleId);

        // ���µ�Ԫ��״̬
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

        // ���¼�����̮���ĵ�Ԫ������
        collapsedCount = 0;
        for (int i = 0; i < height; ++i) for (int j = 0; j < width; ++j) if (grid[i][j]->isCollapsed) collapsedCount++;

        // 3. ����Լ��
        if (!propagate(targetCell->x, targetCell->y)) {
            std::cout << "Propagation led to a contradiction. Backtracking..." << std::endl;
            if (!backtrack()) {
                std::cout << "Backtrack failed. No solution found." << std::endl;
                return false;
            }
            // ���ݺ���Ҫ���¼�����̮���ĵ�Ԫ������
            collapsedCount = 0;
            for (int i = 0; i < height; ++i) for (int j = 0; j < width; ++j) if (grid[i][j]->isCollapsed) collapsedCount++;
        }
    }

    // ����Ƿ����е�Ԫ���ѳɹ�̮��
    if (collapsedCount == width * height) {
        std::cout << "WFC generation successful!" << std::endl;
        return true;
    }
    else {
        std::cout << "WFC generation failed." << std::endl;
        return false;
    }
}