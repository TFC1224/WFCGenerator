#include <SFML/Graphics.hpp>
#include <iostream>
#include <filesystem>
#include <random>
#include <fstream> 
#include <memory>
#include <string> 

// 包含ImGui和其SFML绑定库的头文件
#include "libs/imgui/imgui.h"
#define IMGUI_SFML_API
#include "libs/imgui/imgui-sfml.h"

// 包含项目自定义的头文件
#include "DataManager.h"    // 负责加载和保存项目数据
#include "WFCGenerator.h"   // WFC 算法核心生成器
#include "TileMap.h"        // 用于在 SFML 中渲染瓦片地图

/**
 * @brief 生成地图并更新TileMap对象
 * @param dataManager 数据管理器，提供生成所需的配置
 * @param tileMap 瓦片地图对象，用于加载和显示生成的地图
 * @param status 用于反馈生成状态的字符串引用
 */
void generateAndUpdateMap(
    std::unique_ptr<WFCGenerator>& generator,
    DataManager& dataManager,
    TileMap& tileMap,
    std::string& status,
    std::map<std::string, int>& counts,
    bool forbidCEdge 
)
{
    // 更新状态信息，通知用户正在生成
    status = "生成中... (Generating...)";
    std::cout << "Generating new map..." << std::endl;

    // 创建 WFC 生成器实例，传入网格尺寸和模块定义
    generator.reset(new WFCGenerator(dataManager.gridWidth, dataManager.gridHeight, dataManager.modules));

    // 设置随机种子
    generator->setSeed(dataManager.seed);

    // --- 应用边缘约束 ---
    if (forbidCEdge) {
        for (int y = 0; y < dataManager.gridHeight; ++y) {
            for (int x = 0; x < dataManager.gridWidth; ++x) {
                // 如果单元格在边界上
                if (x == 0 || x == dataManager.gridWidth - 1 || y == 0 || y == dataManager.gridHeight - 1) {
                    generator->removePossibility(x, y, "C"); // 移除'C'的可能性
                }
            }
        }
    }

    // 设置全局模块数量限制
    for (const auto& limit_pair : dataManager.globalLimits) {
        generator->setGlobalModuleLimit(limit_pair.first, limit_pair.second);
    }

    // 运行 WFC 生成算法
    if (generator->generate()) {
        // 如果生成成功
        status = "生成成功！ (Success!)";
        std::cout << "Generation successful!" << std::endl;
        counts = generator->getGlobalModuleCounts(); // 保存计数值
        // 使用生成的网格数据加载并更新 TileMap
        tileMap.load(
            dataManager.tilesetPath, // 瓦片集的路径
            sf::Vector2u(dataManager.tileSize, dataManager.tileSize), // 单个瓦片的尺寸
            generator->getGrid() // 生成的网格数据
        );
    }
    else {
        // 如果生成失败
        counts.clear(); // 生成失败则清空
        status = "生成失败！ (Failed!)";
        std::cout << "Generation failed." << std::endl;
    }
}


int main()
{
    // 创建一个 1200x800 的窗口，标题为 "Modern WFC Generator"
    sf::RenderWindow window(sf::VideoMode(1200, 800), "Modern WFC Generator");
    // 将帧率限制在 60 FPS
    window.setFramerateLimit(60);

    // 初始化 ImGui-SFML 绑定库，false 表示不加载默认字体
    ImGui::SFML::Init(window, false);

    // 检查并加载中文字体文件，以便 ImGui 支持中文显示
    const char* font_path = "assets/msyh.ttc";
    if (std::filesystem::exists(font_path)) {
        ImGuiIO& io = ImGui::GetIO();
        // 从文件加载字体，并指定字体大小和所需的中文字形范围
        io.Fonts->AddFontFromFileTTF(font_path, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        // 更新字体纹理，让 ImGui 可以使用新加载的字体
        ImGui::SFML::UpdateFontTexture();
    }
    else {
        std::cout << "Font not found at " << font_path << std::endl;
    }

    // 创建数据管理器实例
    DataManager dataManager;
    // 从 JSON 文件加载项目设置（模块、约束等）
    if (!dataManager.loadProjectFromFile("wfc_project.json")) {
        std::cerr << "FATAL: Could not load initial project settings!" << std::endl;
        return -1; // 加载失败则退出程序
    }

    // 创建 TileMap 和 View 对象
    TileMap tileMap; // 用于渲染地图
    sf::View view(sf::FloatRect(0, 0, (float)window.getSize().x, (float)window.getSize().y)); // 用于控制地图的视口（平移和缩放）
    sf::Clock deltaClock; // 用于计算 ImGui 更新所需的时间差
    std::string statusMessage = "准备就绪 (Ready)"; // 用于在 UI 中显示状态信息
    std::map<std::string, int> lastGeneratedCounts; //存储上一次成功生成的模块数量
	std::unique_ptr<WFCGenerator> generator;        // 用于存储 WFC 生成器实例

    // 用于控制种子锁定的布尔变量
    bool seedLocked = false;
	// 用于控制是否禁止 'C' 模块出现在边缘的布尔变量
    bool forbid_C_on_edge = false;

    // 主循环，只要窗口打开就一直运行
    while (window.isOpen())
    {
        sf::Event event;
        // 事件处理循环
        while (window.pollEvent(event))
        {
            // 将事件传递给 ImGui 进行处理
            ImGui::SFML::ProcessEvent(window, event);

            // 如果是关闭窗口事件，则关闭窗口
            if (event.type == sf::Event::Closed) window.close();

            // 如果是鼠标滚轮事件，并且鼠标没有悬浮在 ImGui 窗口上
            if (event.type == sf::Event::MouseWheelScrolled && !ImGui::GetIO().WantCaptureMouse)
            {
                // 根据滚轮方向缩放视图
                if (event.mouseWheelScroll.delta > 0) view.zoom(0.9f); // 向上滚动，放大
                else view.zoom(1.1f); // 向下滚动，缩小
            }
        }

        // 如果键盘输入没有被 ImGui 捕获
        if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            // 使用 WASD 键移动视图（平移地图）
            float moveSpeed = 10.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) view.move(0, -moveSpeed);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) view.move(0, moveSpeed);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) view.move(-moveSpeed, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) view.move(moveSpeed, 0);
        }

        // 更新 ImGui，传入自上一帧以来的时间
        ImGui::SFML::Update(window, deltaClock.restart());

        // --- 新增：地块侦测逻辑 ---
        // 检查条件：1. generator必须存在 (已经成功生成过至少一次)
        //          2. 鼠标没有悬浮在任何ImGui窗口上，以防干扰UI
        if (generator && !ImGui::GetIO().WantCaptureMouse)
        {
            // 1. 获取鼠标相对于窗口的像素坐标
            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);

            // 2. 将像素坐标转换为考虑了视图(view)平移和缩放的世界坐标
            sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos, view);

            // 3. 将世界坐标转换为网格坐标
            sf::Vector2i gridPos(
                static_cast<int>(worldPos.x / dataManager.tileSize),
                static_cast<int>(worldPos.y / dataManager.tileSize)
            );

            // 4. 检查网格坐标是否在有效范围内
            if (gridPos.x >= 0 && gridPos.x < dataManager.gridWidth &&
                gridPos.y >= 0 && gridPos.y < dataManager.gridHeight)
            {
                // 5. 获取网格数据并找到对应的单元格
                const auto& grid = generator->getGrid();
                Cell* cell = grid[gridPos.y][gridPos.x];

                if (cell && cell->isCollapsed)
                {
                    // 6. 使用ImGui的Tooltip功能显示信息
                    ImGui::BeginTooltip();
                    ImGui::Text("坐标 (Coords): (%d, %d)", gridPos.x, gridPos.y);
                    ImGui::Text("模块ID (Module ID): %s", cell->chosenModuleId.c_str());
                    ImGui::EndTooltip();
                }
            }
        }

        // --- UI 面板构建 ---
        ImGui::Begin("控制面板"); // 开始一个新的 ImGui 窗口

        // -- 网格设置 --
        if (ImGui::CollapsingHeader("网格设置 (Grid Settings)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SetNextItemWidth(100); // 设置下一个控件的宽度
            ImGui::InputInt("宽度 (Width)", &dataManager.gridWidth);
            ImGui::SameLine(0, 20); // 将下一个控件放在同一行，并设置间距
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("高度 (Height)", &dataManager.gridHeight);
        }

        // -- 随机种子 --
        if (ImGui::CollapsingHeader("随机种子 (Seed)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // "锁定" 复选框
            ImGui::Checkbox("锁定 (Lock)", &seedLocked);
            ImGui::SameLine(); 

            ImGui::BeginDisabled(seedLocked);

            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("##Seed", &dataManager.seed); // "##" 使标签不可见，但ID唯一
            ImGui::SameLine();
            if (ImGui::Button("随机化 (Randomize)")) {
                std::random_device rd;
                std::mt19937 rng(rd());
                std::uniform_int_distribution<int> uni(0, 100000);
                dataManager.seed = uni(rng);
            }

            ImGui::EndDisabled();
        }

        // -- 全局约束 --
        if (ImGui::CollapsingHeader("全局约束 (Global Constraints)"))
        {
            // 使用表格来显示和编辑约束
            if (ImGui::BeginTable("constraints_table", 2, ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("模块 (Module)");
                ImGui::TableSetupColumn("数量上限 (Limit)");
                ImGui::TableHeadersRow();

                // 遍历 dataManager 中的全局限制
                for (auto& pair : dataManager.globalLimits)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", pair.first.c_str()); // 显示模块ID
                    ImGui::TableSetColumnIndex(1);
                    std::string label = "##limit_" + pair.first; 
                    ImGui::SetNextItemWidth(-1); // 让输入框填满整个单元格
                    ImGui::InputInt(label.c_str(), &pair.second); // 创建整数输入框
                }
                ImGui::EndTable();
            }
        }

        if (ImGui::CollapsingHeader("模块权重调整 (Module Weights)"))
        {
            for (auto& module : dataManager.modules) 
            {
                ImGui::InputDouble(module.id.c_str(), &module.weight, 0.1, 0.5, "%.2f");
            }
        }

        ImGui::Checkbox("禁止'C'模块在地图边缘 (Forbid 'C' on edge)", &forbid_C_on_edge);

        ImGui::Separator(); 

        // -- 主操作按钮 --
        if (ImGui::Button("生成新地图 (Generate New Map)", ImVec2(160, 0)))
        {
            // 点击按钮时，调用地图生成函数
            generateAndUpdateMap(
                generator,
                dataManager,
                tileMap,
                statusMessage,
                lastGeneratedCounts,
                forbid_C_on_edge 
            );
        }

        ImGui::SameLine(); //同一行
        if (ImGui::Button("重置视图 (Reset View)"))
        {
            // 将视图的尺寸设置为窗口大小
            view.setSize(sf::Vector2f(window.getSize()));
            // 将视图的中心设置回窗口中心
            view.setCenter(sf::Vector2f(window.getSize()) / 2.f);
        }


        if (ImGui::Button("保存设置 (Save Settings)", ImVec2(160, 0)))
        {
            // 点击按钮时，保存当前设置到文件
            if (dataManager.saveProjectToFile("wfc_project.json")) {
                statusMessage = "设置已保存！ (Settings Saved!)";
            }
            else {
                statusMessage = "保存失败！ (Save Failed!)";
            }
        }
        ImGui::SameLine(); //同一行
        if (ImGui::Button("导出为图片 (Export as Image)", ImVec2(160, 0)))
        {
            // 计算地图的总像素尺寸
            unsigned int mapPixelWidth = dataManager.gridWidth * dataManager.tileSize;
            unsigned int mapPixelHeight = dataManager.gridHeight * dataManager.tileSize;

            // 创建一个渲染纹理
            sf::RenderTexture renderTexture;
            if (renderTexture.create(mapPixelWidth, mapPixelHeight))
            {
                renderTexture.clear(sf::Color(50, 50, 50)); // 设置背景色
                renderTexture.draw(tileMap); // 将tileMap绘制到纹理上
                renderTexture.display();

                // 从纹理中获取图片并保存到文件
                if (renderTexture.getTexture().copyToImage().saveToFile("./output/generated_map.png"))
                {
                    statusMessage = "地图已导出！(Map Exported!)";
                }
                else
                {
                    statusMessage = "导出失败！(Export Failed!)";
                }
            }
        }

        if (ImGui::Button("导出为JSON (Export as JSON)"))
        {
            if (!generator) { 
                statusMessage = "当前无地图数据";
            }
            else {
                nlohmann::json outputJson;
                outputJson["width"] = dataManager.gridWidth;
                outputJson["height"] = dataManager.gridHeight;

                nlohmann::json gridData = nlohmann::json::array();
                const auto& grid = generator->getGrid();

                for (int y = 0; y < dataManager.gridHeight; ++y) {
                    nlohmann::json row = nlohmann::json::array();
                    for (int x = 0; x < dataManager.gridWidth; ++x) {
                        row.push_back(grid[y][x]->chosenModuleId);
                    }
                    gridData.push_back(row);
                }
                outputJson["grid_data"] = gridData;

                std::ofstream file("./output/generated_map.json");
                file << outputJson.dump(4); 
                file.close();

                statusMessage = "地图数据已导出至";
            }
        }

        if (ImGui::CollapsingHeader("预设管理 (Preset Management)"))
        {
            // 使用静态变量来持有文件名缓冲区
            static char filenameBuffer[128] = "wfc_project.json";
            ImGui::InputText("文件名 (Filename)", filenameBuffer, IM_ARRAYSIZE(filenameBuffer));

            if (ImGui::Button("加载预设 (Load Preset)"))
            {
                if (dataManager.loadProjectFromFile(filenameBuffer)) {
                    statusMessage = std::string("已从 ") + filenameBuffer + " 加载设置";
                }
                else {
                    statusMessage = std::string("加载 ") + filenameBuffer + " 失败！";
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("另存为预设 (Save as Preset)"))
            {
                if (dataManager.saveProjectToFile(filenameBuffer)) {
                    statusMessage = std::string("设置已保存至 ") + filenameBuffer;
                }
                else {
                    statusMessage = std::string("保存至 ") + filenameBuffer + " 失败！";
                }
            }
        }

        ImGui::Separator();

        // -- 生成统计 --
        if (ImGui::CollapsingHeader("生成统计 (Generation Stats)"))
        {
            if (lastGeneratedCounts.empty())
            {
                ImGui::Text("暂无数据 (No data yet)");
            }
            else
            {
                if (ImGui::BeginTable("stats_table", 2, ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("模块 (Module)");
                    ImGui::TableSetupColumn("数量 (Count)");
                    ImGui::TableHeadersRow();
                    for (const auto& pair : lastGeneratedCounts)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", pair.first.c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%d", pair.second);
                    }
                    ImGui::EndTable();
                }
            }
        }

        // -- 状态显示 --
        ImGui::Text("状态: %s", statusMessage.c_str());




        ImGui::End(); // 结束 ImGui 窗口

        // --- 渲染 ---
        window.clear(sf::Color(50, 50, 50)); // 用深灰色清空屏幕
        window.setView(view); // 应用自定义视图（用于地图）
        window.draw(tileMap); // 绘制瓦片地图
        window.setView(window.getDefaultView()); // 恢复默认视图（用于UI）
        ImGui::SFML::Render(window); // 渲染 ImGui 界面
        window.display(); // 将后台缓冲区的内容显示到窗口上
    }

    // 关闭并清理 ImGui-SFML
    ImGui::SFML::Shutdown();

    return 0; // 程序正常退出
}