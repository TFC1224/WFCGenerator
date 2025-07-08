#pragma once // ������ָ�ȷ����ͷ�ļ���һ�α�����ֻ������һ��

#include <string>
#include <vector>
#include <map>
#include <SFML/System/Vector2.hpp> 
#include "WFCGenerator.h"         
#include "include/json.hpp"      

// Ϊnlohmann::json���json�ഴ��һ�����̵ı����������������ʹ��
using json = nlohmann::json;

/**
 * @class DataManager
 * @brief �������WFC�㷨������������ݺ����á�
 *
 * ����ദ����ļ��м�����Ŀ���ã�������ߴ硢ģ�鶨�塢Լ���ȣ���
 * ������Щ���ñ�����ļ����������������ļ���WFC�����߼���������
 */
class DataManager {
public:
    // --- WFC �����߼���������� ---

	int gridWidth = 10;     //��������Ŀ�ȣ��Ե�Ԫ��Ϊ��λ����

	int gridHeight = 10;    // ��������ĸ߶ȣ��Ե�Ԫ��Ϊ��λ����

    /**
     * @brief �洢�����Ѽ��ص�ģ�飨Module�������������
     * WFCGenerator ��ʹ����Щģ�������ɵ�ͼ��
     */
    std::vector<Module> modules;

    /**
     * @brief �洢ȫ��ģ���������Ƶ�ӳ�䡣
     * ����ģ���ID (std::string)��ֵ�Ǹ�ģ��������������������ֵ�������� (int)��
     */
    std::map<std::string, int> globalLimits;

    // --- �Ӿ�����Ⱦ��ص����� ---

    /**
     * @brief ������Ƭ��tile�������سߴ硣
     * ���ڴ���Ƭ����tileset������ȷ���и��ÿ��ģ���ͼ��
     */
    int tileSize = 32;

    /**
     * @brief ��Ƭ����tileset��ͼ���ļ���·����
     * ����һ����������ģ���Ӿ���ʾ�ĵ���ͼ���ļ���
     */
    std::string tilesetPath = "assets/tileset.png";

    // --- �ļ��������� ---

    /**
     * @brief ��ָ����JSON�ļ��м���ģ�鶨�塣
     * @param filepath ģ�鶨���ļ���·����
     * @return ������سɹ������� true�����򷵻� false��
     */
    bool loadModulesFromFile(const std::string& filepath);

    /**
     * @brief ��ָ����JSON�ļ��м���������Ŀ���á�
     * ����������ߴ硢���ӡ�Լ���������� loadModulesFromFile �����ض�Ӧ��ģ�顣
     * @param filepath ��Ŀ�����ļ���·����
     * @return ������سɹ������� true�����򷵻� false��
     */
    bool loadProjectFromFile(const std::string& filepath);

    /**
     * @brief ����ǰ����Ŀ���ñ��浽ָ����JSON�ļ���
     * @param filepath Ҫ���浽���ļ���·����
     * @return �������ɹ������� true�����򷵻� false��
     */
    bool saveProjectToFile(const std::string& filepath);

    // --- �������� ---

    /**
     * @brief ����WFC�㷨����������������ӡ�
     * ʹ����ͬ�����ӿ��Ը�����ͬ�����ɽ����
     */
    int seed = 12345;
};