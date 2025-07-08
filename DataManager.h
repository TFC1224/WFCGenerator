#pragma once // 编译器指令，确保该头文件在一次编译中只被包含一次

#include <string>
#include <vector>
#include <map>
#include <SFML/System/Vector2.hpp> 
#include "WFCGenerator.h"         
#include "include/json.hpp"      

// 为nlohmann::json库的json类创建一个更短的别名，方便后续代码使用
using json = nlohmann::json;

/**
 * @class DataManager
 * @brief 负责管理WFC算法所需的所有数据和配置。
 *
 * 这个类处理从文件中加载项目设置（如网格尺寸、模块定义、约束等），
 * 并将这些设置保存回文件。它是连接配置文件和WFC核心逻辑的桥梁。
 */
class DataManager {
public:
    // --- WFC 核心逻辑所需的数据 ---

	int gridWidth = 10;     //生成网格的宽度（以单元格为单位）。

	int gridHeight = 10;    // 生成网格的高度（以单元格为单位）。

    /**
     * @brief 存储所有已加载的模块（Module）定义的向量。
     * WFCGenerator 将使用这些模块来生成地图。
     */
    std::vector<Module> modules;

    /**
     * @brief 存储全局模块数量限制的映射。
     * 键是模块的ID (std::string)，值是该模块在整个网格中允许出现的最大数量 (int)。
     */
    std::map<std::string, int> globalLimits;

    // --- 视觉和渲染相关的数据 ---

    /**
     * @brief 单个瓦片（tile）的像素尺寸。
     * 用于从瓦片集（tileset）中正确地切割出每个模块的图像。
     */
    int tileSize = 32;

    /**
     * @brief 瓦片集（tileset）图像文件的路径。
     * 这是一个包含所有模块视觉表示的单个图像文件。
     */
    std::string tilesetPath = "assets/tileset.png";

    // --- 文件操作函数 ---

    /**
     * @brief 从指定的JSON文件中加载模块定义。
     * @param filepath 模块定义文件的路径。
     * @return 如果加载成功，返回 true；否则返回 false。
     */
    bool loadModulesFromFile(const std::string& filepath);

    /**
     * @brief 从指定的JSON文件中加载整个项目配置。
     * 这会加载网格尺寸、种子、约束，并调用 loadModulesFromFile 来加载对应的模块。
     * @param filepath 项目配置文件的路径。
     * @return 如果加载成功，返回 true；否则返回 false。
     */
    bool loadProjectFromFile(const std::string& filepath);

    /**
     * @brief 将当前的项目配置保存到指定的JSON文件。
     * @param filepath 要保存到的文件的路径。
     * @return 如果保存成功，返回 true；否则返回 false。
     */
    bool saveProjectToFile(const std::string& filepath);

    // --- 其他配置 ---

    /**
     * @brief 用于WFC算法的随机数生成器种子。
     * 使用相同的种子可以复现相同的生成结果。
     */
    int seed = 12345;
};