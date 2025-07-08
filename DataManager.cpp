#include "DataManager.h"
#include <fstream>      
#include <stdexcept>   
#include <iostream>   

/**
 * @brief ��ģ�鶨��JSON�ļ��м�������ģ�鼰���Ӿ����ڽӹ���
 * @param filepath ģ���ļ���·����
 * @return ������سɹ������� true�����򷵻� false��
 */
bool DataManager::loadModulesFromFile(const std::string& filepath) {
    // ���Դ��ļ�
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open module file: " << filepath << std::endl;
        return false;
    }

    try {
        // ʹ�� nlohmann/json ������ļ�����
        json data = json::parse(file);

        // ������Ϣ�����JSON��û����ʹ��Ĭ��ֵ
        tileSize = data.value("tile_size", 32); // ��ȡ��Ƭ�ߴ磬Ĭ��Ϊ32
        tilesetPath = data.value("tileset_path", "assets/tileset.png"); // ��ȡ��Ƭ��·����Ĭ��Ϊ "assets/tileset.png"

        modules.clear(); // �ڼ���������ǰ����վɵ�ģ������

        // ����JSON�ļ��� "modules" �����ÿ��ģ�����
        for (const auto& mod_json : data["modules"]) {
            // ��ȡģ��ID��Ȩ�أ������������ʹ��Ĭ��ֵ
            std::string id = mod_json.value("id", "NO_ID");
            double weight = mod_json.value("weight", 1.0);

            // ����һ���µ� Module ʵ��
            Module module(id, weight);

            // ���ظ�ģ������ͼ�ϵ����� (tile_index)
            if (mod_json.contains("tile_index") && mod_json["tile_index"].is_array() && mod_json["tile_index"].size() == 2) {
                module.tileIndex.y = mod_json["tile_index"][0]; // �����һ��Ԫ������ (y)
                module.tileIndex.x = mod_json["tile_index"][1]; // ����ڶ���Ԫ������ (x)
            }

            // �����ڽӹ��� (adjacency)
            if (mod_json.contains("adjacency")) {
                // �����ڽӹ�������е�ÿһ�Լ�ֵ������ -> �����ģ��ID�б�
                for (auto const& [key, val] : mod_json["adjacency"].items()) {
                    Direction dir;
                    // ��JSON�е��ַ�������ת��Ϊö������
                    if (key == "TOP") dir = TOP;
                    else if (key == "BOTTOM") dir = BOTTOM;
                    else if (key == "LEFT") dir = LEFT;
                    else dir = RIGHT;

                    // ����һ���������洢�÷��������������ģ��ID
                    std::set<std::string> allowed_modules;
                    for (const auto& allowed_id : val) {
                        allowed_modules.insert(allowed_id.get<std::string>());
                    }
                    // ���������Ĺ������ģ��
                    module.adjacencyRules[dir] = allowed_modules;
                }
            }
            // ����ȫ���úõ�ģ����ӵ��������б���
            modules.push_back(module);
        }
    }
    catch (json::parse_error& e) {
        // ���JSON����ʧ�ܣ������쳣����ӡ������Ϣ
        std::cerr << "JSON parse error in " << filepath << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief ����Ŀ�����ļ�����������Ŀ�����á�
 * ���������ߴ硢ȫ��Լ�������ᴥ�����ض�Ӧ��ģ���ļ���
 * @param filepath ��Ŀ�ļ���·����
 * @return ������سɹ������� true�����򷵻� false��
 */
bool DataManager::loadProjectFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open project file: " << filepath << std::endl;
        return false;
    }
    try {
        json data = json::parse(file);
        // ���������Ⱥ͸߶ȣ������������ʹ��Ĭ��ֵ
        gridWidth = data.value("grid_width", 10);
        gridHeight = data.value("grid_height", 10);

        // ����Ŀ�����ж�ȡģ���ļ���·����Ȼ����ú���������
        std::string module_source = data.value("module_source", "wfc_modules.json");
        if (!loadModulesFromFile(module_source)) {
            // ���ģ���ļ�����ʧ�ܣ���������Ŀ����Ҳʧ��
            return false;
        }

        globalLimits.clear(); // ��վɵ�ȫ��Լ��
        // ����Ƿ����ȫ��Լ������
        if (data.contains("global_constraints")) {
            // ����Լ������
            for (const auto& constraint : data["global_constraints"]) {
                // ��Լ����ģ��ID -> �������ޣ�����map
                globalLimits[constraint["id"]] = constraint["limit"];
            }
        }

        seed = data.value("seed", 12345); // ��������ֵ��Ĭ����12345
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON parse error in " << filepath << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief ����ǰ����Ŀ���ñ��浽�ļ���
 * @param filepath Ҫ���浽����Ŀ�ļ���·����
 * @return �������ɹ������� true�����򷵻� false��
 */
bool DataManager::saveProjectToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open project file for writing: " << filepath << std::endl;
        return false;
    }

    try {
        // ����һ���յ�json����
        json data;
        // ����ǰ����д��json����
        data["grid_width"] = gridWidth;
        data["grid_height"] = gridHeight;
        data["seed"] = seed;
        data["module_source"] = "wfc_modules.json"; // ����ģ���ļ�������

        // �� globalLimits map ת���� json array of objects
        json constraints_array = json::array();
        for (const auto& pair : globalLimits) {
            json constraint_obj;
            constraint_obj["id"] = pair.first;
            constraint_obj["limit"] = pair.second;
            constraints_array.push_back(constraint_obj);
        }
        data["global_constraints"] = constraints_array;

        // ��json����д���ļ�
        file << data.dump(4);
    }
    catch (const std::exception& e) {
		// �쳣�������������Ϣ
        std::cerr << "Error while saving to JSON: " << e.what() << std::endl;
        return false;
    }

    return true;
}