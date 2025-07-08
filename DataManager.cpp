#include "DataManager.h"
#include <fstream>      
#include <stdexcept>   
#include <iostream>   

/**
 * @brief 从模块定义JSON文件中加载所有模块及其视觉和邻接规则。
 * @param filepath 模块文件的路径。
 * @return 如果加载成功，返回 true；否则返回 false。
 */
bool DataManager::loadModulesFromFile(const std::string& filepath) {
    // 尝试打开文件
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open module file: " << filepath << std::endl;
        return false;
    }

    try {
        // 使用 nlohmann/json 库解析文件内容
        json data = json::parse(file);

        // 加载信息，如果JSON中没有则使用默认值
        tileSize = data.value("tile_size", 32); // 获取瓦片尺寸，默认为32
        tilesetPath = data.value("tileset_path", "assets/tileset.png"); // 获取瓦片集路径，默认为 "assets/tileset.png"

        modules.clear(); // 在加载新数据前，清空旧的模块数据

        // 遍历JSON文件中 "modules" 数组的每个模块对象
        for (const auto& mod_json : data["modules"]) {
            // 获取模块ID和权重，如果不存在则使用默认值
            std::string id = mod_json.value("id", "NO_ID");
            double weight = mod_json.value("weight", 1.0);

            // 创建一个新的 Module 实例
            Module module(id, weight);

            // 加载该模块在贴图上的坐标 (tile_index)
            if (mod_json.contains("tile_index") && mod_json["tile_index"].is_array() && mod_json["tile_index"].size() == 2) {
                module.tileIndex.y = mod_json["tile_index"][0]; // 数组第一个元素是行 (y)
                module.tileIndex.x = mod_json["tile_index"][1]; // 数组第二个元素是列 (x)
            }

            // 加载邻接规则 (adjacency)
            if (mod_json.contains("adjacency")) {
                // 遍历邻接规则对象中的每一对键值（方向 -> 允许的模块ID列表）
                for (auto const& [key, val] : mod_json["adjacency"].items()) {
                    Direction dir;
                    // 将JSON中的字符串方向转换为枚举类型
                    if (key == "TOP") dir = TOP;
                    else if (key == "BOTTOM") dir = BOTTOM;
                    else if (key == "LEFT") dir = LEFT;
                    else dir = RIGHT;

                    // 创建一个集合来存储该方向上所有允许的模块ID
                    std::set<std::string> allowed_modules;
                    for (const auto& allowed_id : val) {
                        allowed_modules.insert(allowed_id.get<std::string>());
                    }
                    // 将解析出的规则存入模块
                    module.adjacencyRules[dir] = allowed_modules;
                }
            }
            // 将完全配置好的模块添加到管理器列表中
            modules.push_back(module);
        }
    }
    catch (json::parse_error& e) {
        // 如果JSON解析失败，捕获异常并打印错误信息
        std::cerr << "JSON parse error in " << filepath << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief 从项目配置文件加载整个项目的设置。
 * 这包括网格尺寸、全局约束，并会触发加载对应的模块文件。
 * @param filepath 项目文件的路径。
 * @return 如果加载成功，返回 true；否则返回 false。
 */
bool DataManager::loadProjectFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open project file: " << filepath << std::endl;
        return false;
    }
    try {
        json data = json::parse(file);
        // 加载网格宽度和高度，如果不存在则使用默认值
        gridWidth = data.value("grid_width", 10);
        gridHeight = data.value("grid_height", 10);

        // 从项目配置中读取模块文件的路径，然后调用函数加载它
        std::string module_source = data.value("module_source", "wfc_modules.json");
        if (!loadModulesFromFile(module_source)) {
            // 如果模块文件加载失败，则整个项目加载也失败
            return false;
        }

        globalLimits.clear(); // 清空旧的全局约束
        // 检查是否存在全局约束定义
        if (data.contains("global_constraints")) {
            // 遍历约束数组
            for (const auto& constraint : data["global_constraints"]) {
                // 将约束（模块ID -> 数量上限）存入map
                globalLimits[constraint["id"]] = constraint["limit"];
            }
        }

        seed = data.value("seed", 12345); // 加载种子值，默认是12345
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON parse error in " << filepath << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief 将当前的项目设置保存到文件。
 * @param filepath 要保存到的项目文件的路径。
 * @return 如果保存成功，返回 true；否则返回 false。
 */
bool DataManager::saveProjectToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open project file for writing: " << filepath << std::endl;
        return false;
    }

    try {
        // 创建一个空的json对象
        json data;
        // 将当前设置写入json对象
        data["grid_width"] = gridWidth;
        data["grid_height"] = gridHeight;
        data["seed"] = seed;
        data["module_source"] = "wfc_modules.json"; // 假设模块文件名不变

        // 将 globalLimits map 转换回 json array of objects
        json constraints_array = json::array();
        for (const auto& pair : globalLimits) {
            json constraint_obj;
            constraint_obj["id"] = pair.first;
            constraint_obj["limit"] = pair.second;
            constraints_array.push_back(constraint_obj);
        }
        data["global_constraints"] = constraints_array;

        // 将json对象写入文件
        file << data.dump(4);
    }
    catch (const std::exception& e) {
		// 异常捕获，输出错误信息
        std::cerr << "Error while saving to JSON: " << e.what() << std::endl;
        return false;
    }

    return true;
}