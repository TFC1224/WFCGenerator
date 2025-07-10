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
 * @brief �����ĸ���������
 * COUNT ���ڻ�ȡ�����������
 */
enum Direction {
    TOP,    // ��
    BOTTOM, // ��
    LEFT,   // ��
    RIGHT,  // ��
    COUNT   // ��������
};

// ������������
std::string directionToString(Direction dir);

/**
 * @class Module
 * @brief ����һ�����ɵ�Ԫ������Ƭ����
 * ÿ��ģ�鶼������ص�ID��Ȩ�ء��ڽӹ��������Ƭ���ϵ�λ�á�
 */
class Module {
public:
    std::string id;                                     // ģ���Ψһ��ʶ��
    double weight;                                      // ģ����ѡ��ʱ��Ȩ�أ�Ӱ�������Ƶ��
    std::map<Direction, std::set<std::string>> adjacencyRules; // �ڽӹ��򣬶������ڸ��������Ͽ�������Щģ��ID����
    sf::Vector2i tileIndex;                             // ģ������Ƭ����tileset�������ϵ���������

    /**
     * @brief Module ���캯����
     * @param id ģ��ID��
     * @param weight ģ��Ȩ�ء�
     */
    Module(std::string id, double weight) : id(id), weight(weight), tileIndex(0, 0) {}

    /**
     * @brief ��鵱ǰģ���Ƿ������ָ������������һ��ģ����ݡ�
     * ��������˫��ģ�A���Ϸ������ܽ�B��ͬʱB���·�Ҳ�����ܽ�A��
     * @param dir ����ڵ�ǰģ��ķ���
     * @param otherModule Ҫ�������Ե���һ��ģ�顣
     * @return ��������򷵻� true�����򷵻� false��
     */
    bool isCompatible(Direction dir, const Module& otherModule) const {
        Direction oppositeDir;
        if (dir == TOP) oppositeDir = BOTTOM;
        else if (dir == BOTTOM) oppositeDir = TOP;
        else if (dir == LEFT) oppositeDir = RIGHT;
        else oppositeDir = LEFT;

        // ���˫���Ƿ񶼶����˶�Ӧ����Ĺ���
        if (adjacencyRules.count(dir) == 0 || otherModule.adjacencyRules.count(oppositeDir) == 0) {
            return false;
        }

        // �������Ƿ��໥ƥ��
        return adjacencyRules.at(dir).count(otherModule.id) &&
            otherModule.adjacencyRules.at(oppositeDir).count(this->id);
    }
};

/**
 * @class Cell
 * @brief ������������е�һ����Ԫ��
 * ÿ����Ԫ��ά��һ����ܵ�ģ�飬ֱ��������̮����Ϊһ��ȷ����ģ�顣
 */
class Cell {
public:
    int x, y;                               // ��Ԫ���������е�����
    bool isCollapsed;                       // ��ǵ�Ԫ���Ƿ���̮��
    std::set<std::string> possibleModules;  // �洢��ǰ��Ԫ�����п��ܵ�ģ��ID����
    std::string chosenModuleId;             // ̮����ѡ����ģ��ID
    const Module* module = nullptr;         // ָ��̮����ѡ����ģ������ָ��


    /**
     * @brief Cell ���캯����
     * @param x X���ꡣ
     * @param y Y���ꡣ
     * @param allModules ���п���ģ����б����ڳ�ʼ�� possibleModules��
     */
    Cell(int x, int y, const std::vector<Module>& allModules) : x(x), y(y), isCollapsed(false) {
        for (const auto& module : allModules) {
            possibleModules.insert(module.id);
        }
    }

    /**
     * @brief ���㵥Ԫ����ء�
     * ��WFC�У���ͨ���ǿ���ģ�����������Խ�ͣ���ȷ����ԽС��
     * @return ����ģ���������
     */
    size_t calculateEntropy() const {
        return possibleModules.size();
    }

    /**
     * @brief �ӿ���ģ�鼯�����Ƴ�һ��ģ��ID��
     * ����WFC�����׶εĺ��Ĳ�����
     * @param moduleId Ҫ�Ƴ���ģ��ID��
     * @return ����ɹ��Ƴ���ģ�飬�򷵻� true��
     */
    bool removePossibleModule(const std::string& moduleId) {
        if (possibleModules.count(moduleId)) {
            possibleModules.erase(moduleId);
            return true;
        }
        return false;
    }

    /**
     * @brief ���õ�Ԫ��״̬��
     * ���ڻ���ʱ����Ԫ��ָ���֮ǰ��ĳ��״̬��
     * @param initialPossibleModules Ҫ�ָ����Ŀ���ģ�鼯�ϡ�
     */
    void reset(const std::set<std::string>& initialPossibleModules) {
        isCollapsed = false;
        chosenModuleId = "";
        possibleModules = initialPossibleModules;
    }
};

/**
 * @struct StateSnapshot
 * @brief �����ڻ���ʱ����ͻָ��㷨�Ĺؼ�״̬��
 * ��ѡ��һ��ģ�����̮��ʱ���ᴴ��һ�����ա������������ʧ�ܣ���ʹ�ô˿��ս��л��ݡ�
 */
struct StateSnapshot {
    int cellX, cellY;                                           // ����̮���ĵ�Ԫ������
    std::set<std::string> initialPossibleModules;               // ̮��ǰ�õ�Ԫ��Ŀ���ģ��
    std::string attemptedModuleId;                              // ����̮���ɵ�ģ��ID
    std::map<std::pair<int, int>, std::set<std::string>> gridStateSnapshot; // ����������̮��ǰ��״̬
    std::map<std::string, int> globalModuleCountsSnapshot;      // ȫ��ģ�������̮��ǰ��״̬

    /**
     * @brief StateSnapshot ���캯����
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
 * @brief ������̮����WFC���㷨�ĺ���ʵ���ࡣ
 * ��������ģ�顢���ɹ��̡����ݵȡ�
 */
class WFCGenerator {
public:
    /**
     * @brief WFCGenerator ���캯����
     * @param width ��������Ŀ�ȡ�
     * @param height ��������ĸ߶ȡ�
     * @param modules �������ɵ�����ģ����б�
     */
    WFCGenerator(int width, int height, const std::vector<Module>& modules);

    /**
     * @brief WFCGenerator ����������
     * �����ͷ������ж�̬����� Cell ����
     */
    ~WFCGenerator();

    /**
     * @brief �����ض�ģ�������������е��������ޡ�
     * @param moduleId Ҫ���Ƶ�ģ��ID��
     * @param limit �������ޡ�
     */
    void setGlobalModuleLimit(const std::string& moduleId, int limit);

    /**
     * @brief ����WFC���ɹ��̡�
     * @return ����ɹ��������������򷵻� true�����򷵻� false��
     */
    bool generate(bool useRelaxation = false);

    /**
     * @brief ��ȡ���ɵ��������ݡ�
     * @return һ���������ã�ָ��洢 Cell ָ��Ķ�ά������
     */
    const std::vector<std::vector<Cell*>>& getGrid() const;

    const std::map<std::string, int>& getGlobalModuleCounts() const;

    /**
     * @brief �ڿ���̨��ӡ���ɵ��������ڵ��ԣ���
     */
    void printGrid() const;

    /**
     * @brief ����������������ɵ����ӡ�
     * @param seed ��������ӡ�
     */
    void setSeed(unsigned int seed);

    void removePossibility(int x, int y, const std::string& moduleId);

private:
    int width, height;                                  // ����ߴ�
    std::vector<std::vector<Cell*>> grid;               // �洢����Ԫ��Ķ�ά����
    std::vector<Module> allModules;                     // ���п���ģ��ĸ���
    std::map<std::string, int> globalModuleCounts;      // ��ǰ�����и�ģ��ļ���
    std::map<std::string, int> globalModuleLimits;      // ��ģ���ȫ����������
    std::stack<StateSnapshot> stateStack;               // ���ڻ��ݵ�״̬����ջ
    std::mt19937 gen;                                   // �����������

    // ˽�и�������
    void initializeGrid();                              // ��ʼ�����񣬴������� Cell ����
    Cell* getLowestEntropyCell();                       // ���Ҳ���������ͣ��ȷ������δ̮����Ԫ��
    bool collapseCell(Cell* cell, std::string& chosenModuleId); // ̮��һ����Ԫ��Ϊ��ѡ��һ��ȷ����ģ��
    bool propagate(int startX, int startY);             // ��һ���㿪ʼ�����⴫��Լ���������ھӵ�Ԫ��Ŀ���ģ��
    void saveState(Cell* cellToCollapse, const std::string& chosenModuleId); // ���浱ǰ״̬������ջ
    bool backtrack();                                   // ִ�л��ݣ��ָ�����һ��״̬����������ѡ��
};