#include <SFML/Graphics.hpp>
#include <iostream>
#include <filesystem>
#include <random>
#include <fstream> 
#include <memory>
#include <string> 

#include "libs/imgui/imgui.h"
#define IMGUI_SFML_API
#include "libs/imgui/imgui-sfml.h"

#include "DataManager.h"    
#include "WFCGenerator.h"   
#include "TileMap.h"       

bool generateAndUpdateMap(
    std::unique_ptr<WFCGenerator>& generator,
    DataManager& dataManager,
    TileMap& tileMap,
    std::string& status,
    std::map<std::string, int>& counts,
    bool forbidCEdge,
    bool enableConstraintRelaxation = false,
    bool useHeuristics = false
)
{
    status = "生成中... (Generating...)";
    std::cout << "Generating new map..." << std::endl;

    generator.reset(new WFCGenerator(dataManager.gridWidth, dataManager.gridHeight, dataManager.modules));

    generator->setSeed(dataManager.seed);

    generator->setHeuristicTieBreaking(useHeuristics);

    if (forbidCEdge) {
        for (int y = 0; y < dataManager.gridHeight; ++y) {
            for (int x = 0; x < dataManager.gridWidth; ++x) {
                if (x == 0 || x == dataManager.gridWidth - 1 || y == 0 || y == dataManager.gridHeight - 1) {
                    generator->removePossibility(x, y, "C"); 
                }
            }
        }
    }

    for (const auto& limit_pair : dataManager.globalLimits) {
        generator->setGlobalModuleLimit(limit_pair.first, limit_pair.second);
    }

    if (generator->generate(enableConstraintRelaxation)) {
        status = "生成成功！ (Success!)";
        std::cout << "Generation successful!" << std::endl;
        counts = generator->getGlobalModuleCounts(); 
        tileMap.load(
            dataManager.tilesetPath, 
            sf::Vector2u(dataManager.tileSize, dataManager.tileSize), 
            generator->getGrid() 
        );
        return true;
    }
    else {
        counts.clear(); 
        status = "生成失败！ (Failed!)";
        std::cout << "Generation failed." << std::endl;
        return false;
    }
}

bool validateParkRule(const std::unique_ptr<WFCGenerator>& generator, DataManager& dataManager) {
    const auto& grid = generator->getGrid();
    for (int y = 0; y < dataManager.gridHeight; ++y) {
        for (int x = 0; x < dataManager.gridWidth; ++x) {
            if (grid[y][x]->chosenModuleId == "P") { 
                bool hasRoadNeighbor = false;

                int dx[] = { 0, 0, -1, 1 };
                int dy[] = { -1, 1, 0, 0 };
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    if (nx >= 0 && nx < dataManager.gridWidth && ny >= 0 && ny < dataManager.gridHeight) {
                        if (grid[ny][nx]->chosenModuleId == "R") {
                            hasRoadNeighbor = true;
                            break; 
                        }
                    }
                }
                if (!hasRoadNeighbor) {
                    return false; 
                }
            }
        }
    }
    return true; 
}

bool validateCommercialClustering(const std::unique_ptr<WFCGenerator>& generator, DataManager& dataManager) {
    const auto& grid = generator->getGrid();
    for (int y = 0; y < dataManager.gridHeight; ++y) {
        for (int x = 0; x < dataManager.gridWidth; ++x) {
            if (grid[y][x]->chosenModuleId == "C") { 
                bool hasCommercialNeighbor = false;
                int dx[] = { 0, 0, -1, 1 };
                int dy[] = { -1, 1, 0, 0 };
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    if (nx >= 0 && nx < dataManager.gridWidth && ny >= 0 && ny < dataManager.gridHeight) {
                        if (grid[ny][nx]->chosenModuleId == "C") {
                            hasCommercialNeighbor = true;
                            break;
                        }
                    }
                }
                if (!hasCommercialNeighbor) {
                    return false; 
                }
            }
        }
    }
    return true; 
}

bool validateHousingAccessibility(const std::unique_ptr<WFCGenerator>& generator, DataManager& dataManager) {
    const auto& grid = generator->getGrid();
    for (int y = 0; y < dataManager.gridHeight; ++y) {
        for (int x = 0; x < dataManager.gridWidth; ++x) {
            if (grid[y][x]->chosenModuleId == "H") { 
                bool hasRoadNeighbor = false;
                int dx[] = { 0, 0, -1, 1 };
                int dy[] = { -1, 1, 0, 0 };
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    if (nx >= 0 && nx < dataManager.gridWidth && ny >= 0 && ny < dataManager.gridHeight) {
                        if (grid[ny][nx]->chosenModuleId == "R") {
                            hasRoadNeighbor = true;
                            break;
                        }
                    }
                }
                if (!hasRoadNeighbor) {
                    return false; 
                }
            }
        }
    }
    return true; 
}

void fitViewToMap(sf::View& view, const sf::RenderWindow& window, const DataManager& dataManager)
{
    const float panelWidth = 350.f; 
    float mapWidthPx = static_cast<float>(dataManager.gridWidth * dataManager.tileSize);
    float mapHeightPx = static_cast<float>(dataManager.gridHeight * dataManager.tileSize);
    if (mapWidthPx == 0 || mapHeightPx == 0) return; 

    float viewAreaWidth = static_cast<float>(window.getSize().x) - panelWidth;
    float viewAreaHeight = static_cast<float>(window.getSize().y);
    if (viewAreaWidth <= 0 || viewAreaHeight <= 0) return;

    float mapAspectRatio = mapWidthPx / mapHeightPx;
    float viewAreaAspectRatio = viewAreaWidth / viewAreaHeight;

    sf::Vector2f viewSize;
    if (mapAspectRatio > viewAreaAspectRatio) {
        viewSize = { mapWidthPx, mapWidthPx / viewAreaAspectRatio };
    }
    else {
        viewSize = { mapHeightPx * viewAreaAspectRatio, mapHeightPx };
    }
    view.setSize(viewSize);

    view.zoom(1.1f);

    view.setCenter(mapWidthPx / 2.f, mapHeightPx / 2.f);

    float windowWidth = static_cast<float>(window.getSize().x);
    float viewportWidthRatio = viewAreaWidth / windowWidth;
    view.setViewport(sf::FloatRect(0.f, 0.f, viewportWidthRatio, 1.f));
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(1200, 800), "WFC Generator");

    window.setFramerateLimit(60);

    ImGui::SFML::Init(window, false);

    const char* font_path = "assets/msyh.ttc";
    if (std::filesystem::exists(font_path)) {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF(font_path, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        ImGui::SFML::UpdateFontTexture();
    }
    else {
        std::cout << "Font not found at " << font_path << std::endl;
    }

    DataManager dataManager;

    if (!dataManager.loadProjectFromFile("wfc_project.json")) {
        std::cerr << "FATAL: Could not load initial project settings!" << std::endl;
        return -1;
    }

    // --- 加载背景 ---
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("assets/background.png")) {
        std::cerr << "Error loading background texture!" << std::endl;
    }
    backgroundTexture.setRepeated(true);
    sf::Sprite backgroundSprite(backgroundTexture);

    TileMap tileMap; // 用于渲染地图
    sf::View view(sf::FloatRect(0, 0, (float)window.getSize().x, (float)window.getSize().y)); // 用于控制地图的视口（平移和缩放）
    sf::Clock deltaClock; // 用于计算 ImGui 更新所需的时间差
	sf::Clock animationClock; // 用于控制动画帧率
    std::string statusMessage = "准备就绪 (Ready)"; // 用于在 UI 中显示状态信息
    std::map<std::string, int> lastGeneratedCounts; //存储上一次成功生成的模块数量
	std::unique_ptr<WFCGenerator> generator;        // 用于存储 WFC 生成器实例

    // 用于控制种子锁定的布尔变量
    bool seedLocked = false;
	// 用于控制是否禁止 'C' 模块出现在边缘的布尔变量
    bool forbid_C_on_edge = false;
	// 用于控制公园模块是否需要有道路邻居的布尔变量
    bool require_park_has_road_neighbor = false;
	// 用于控制商业区模块是否需要聚集的布尔变量
    bool require_commercial_clustering = false;
    // 用于控制房屋模块是否需要有道路邻居的布尔变量
    bool require_housing_accessibility = false;
    // 用于控制最大生成次数
    int maxTries = 6;
	// 用于控制是否需要在生成时重置视图
    bool needsViewResetOnGenerate = false;
	// 用于控制是否启用约束松弛
    bool enableConstraintRelaxation = false;
	// 用于控制是否使用启发式择优
    bool useHeuristicTieBreaking = false;

    while (window.isOpen())
    {
        sf::Event event;

        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::MouseWheelScrolled && !ImGui::GetIO().WantCaptureMouse)
            {
                if (event.mouseWheelScroll.delta > 0) view.zoom(0.9f); 
                else view.zoom(1.1f); 
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

        const float panelWidth = 350.f; 
        ImGui::SetNextWindowPos(ImVec2(window.getSize().x - panelWidth, 0.f));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, static_cast<float>(window.getSize().y)));
        ImGui::SetNextWindowBgAlpha(0.9f);

        // --- 地块侦测逻辑 ---
        if (generator && !ImGui::GetIO().WantCaptureMouse)
        {
            sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
            const float panelWidth = 350.f;

            if (generator && !ImGui::GetIO().WantCaptureMouse && pixelPos.x < (window.getSize().x - panelWidth)) 
            {
                sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos, view);

                sf::Vector2i gridPos(
                    static_cast<int>(worldPos.x / dataManager.tileSize),
                    static_cast<int>(worldPos.y / dataManager.tileSize)
                );

                if (gridPos.x >= 0 && gridPos.x < dataManager.gridWidth &&
                    gridPos.y >= 0 && gridPos.y < dataManager.gridHeight)
                {
                    const auto& grid = generator->getGrid();
                    Cell* cell = grid[gridPos.y][gridPos.x];

                    if (cell && cell->isCollapsed)
                    {
                        sf::Vector2f rectTopLeft_world(static_cast<float>(gridPos.x* dataManager.tileSize), static_cast<float>(gridPos.y* dataManager.tileSize));
                        sf::Vector2f rectBottomRight_world(rectTopLeft_world.x + dataManager.tileSize, rectTopLeft_world.y + dataManager.tileSize);

                        sf::Vector2i rectTopLeft_screen = window.mapCoordsToPixel(rectTopLeft_world, view);
                        sf::Vector2i rectBottomRight_screen = window.mapCoordsToPixel(rectBottomRight_world, view);

                        ImGui::GetBackgroundDrawList()->AddRectFilled(
                            ImVec2(static_cast<float>(rectTopLeft_screen.x), static_cast<float>(rectTopLeft_screen.y)),
                            ImVec2(static_cast<float>(rectBottomRight_screen.x), static_cast<float>(rectBottomRight_screen.y)),
                            IM_COL32(255, 255, 0, 80) 
                        );

                        ImGui::SetNextWindowPos(ImVec2(pixelPos.x + 15, pixelPos.y + 15));

                        ImGui::SetNextWindowSize(ImVec2(0, 0));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));

                        ImGui::BeginTooltip();
                        ImGui::Text("坐标 (Coords): (%d, %d)", gridPos.x, gridPos.y);
                        ImGui::Text("模块ID (Module ID): %s", cell->chosenModuleId.c_str());
                        ImGui::EndTooltip();

                        ImGui::PopStyleVar();
                    }
                }
            }
        }

        // --- UI 面板构建 ---
        ImGui::Begin(
            "控制面板",
            nullptr, 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse 
        );

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

        ImGui::Separator();

        if (ImGui::CollapsingHeader("高级生成规则 (Advanced Rules)")) 
        {
            ImGui::SetNextItemWidth(100); 
            ImGui::InputInt("最大尝试次数 (Max Tries)", &maxTries);
            ImGui::Separator(); 

            ImGui::Checkbox("禁止商业区在地图边缘 (Forbid Commercia on edge)", &forbid_C_on_edge);

            ImGui::Checkbox("公园必须临路 (Park must have Road neighbor)", &require_park_has_road_neighbor);

            ImGui::Checkbox("商业区必须聚集 (Commercial must cluster)", &require_commercial_clustering);

            ImGui::Checkbox("住房必须临路 (Housing must be accessible)", &require_housing_accessibility);

            ImGui::Checkbox("启用柔性约束 (Enable Constraint Relaxation)", &enableConstraintRelaxation);

            ImGui::Checkbox("启用启发式择优 (Enable Heuristic Tie-Breaking)", &useHeuristicTieBreaking);

            ImGui::Separator();
        }

        ImGui::Separator();


        // -- 主操作按钮 --
        if (ImGui::Button("生成新地图 (Generate New Map)", ImVec2(160, 0)))
        {
            // 检查规则是否启用
            bool useRejectionSampling = require_park_has_road_neighbor || require_commercial_clustering || require_housing_accessibility;

            if (!useRejectionSampling)
            {
                if (generateAndUpdateMap(
                    generator,
                    dataManager,
                    tileMap,
                    statusMessage,
                    lastGeneratedCounts,
                    forbid_C_on_edge,
                    enableConstraintRelaxation,
                    useHeuristicTieBreaking
                ))
                {
                    needsViewResetOnGenerate = true;
                }
            }
            else 
            {
                bool success = false;
                statusMessage = "生成中 (满足特殊规则)...";
                for (int i = 0; i < maxTries; ++i)
                {
                    // 随机种子生成
                    std::random_device rd;
                    std::mt19937 rng(rd());
                    std::uniform_int_distribution<int> uni(0, 100000);
                    dataManager.seed = uni(rng);

                    generateAndUpdateMap(
                        generator,
                        dataManager,
                        tileMap,
                        statusMessage,
                        lastGeneratedCounts,
                        forbid_C_on_edge,
                        enableConstraintRelaxation,
                        useHeuristicTieBreaking
                    );

                    if (generator && generator->getGrid()[0][0]->isCollapsed)
                    {
                        // 依次检查所有被勾选的规则
                        bool passesAllChecks = true;
                        if (require_park_has_road_neighbor && !validateParkRule(generator, dataManager)) {
                            passesAllChecks = false;
                        }
                        if (passesAllChecks && require_commercial_clustering && !validateCommercialClustering(generator, dataManager)) {
                            passesAllChecks = false;
                        }
                        if (passesAllChecks && require_housing_accessibility && !validateHousingAccessibility(generator, dataManager)) {
                            passesAllChecks = false;
                        }

                        // 如果所有启用的规则都通过了检查
                        if (passesAllChecks) {
                            statusMessage = "生成成功 (已满足所有特殊规则)!";
                            success = true;
                            needsViewResetOnGenerate = true;
                            break; 
                        }
                    }
                }
                if (!success) {
                    statusMessage = "生成失败次数过多无法满足特殊规则! (Failed to meet special rules!)";
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("重置视图 (Reset View)"))
        {
            if (generator) { 
                fitViewToMap(view, window, dataManager);
            }
        }

        if (ImGui::CollapsingHeader("导出地图信息 (Export map information)"))
        {
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
            ImGui::SameLine();
            if (ImGui::Button("导出为图片 (Export as Image)", ImVec2(160, 0)))
            {
                unsigned int mapPixelWidth = dataManager.gridWidth * dataManager.tileSize;
                unsigned int mapPixelHeight = dataManager.gridHeight * dataManager.tileSize;

                sf::RenderTexture renderTexture;
                if (renderTexture.create(mapPixelWidth, mapPixelHeight))
                {
                    renderTexture.clear(sf::Color(50, 50, 50));
                    renderTexture.draw(tileMap);
                    renderTexture.display();

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
        }
        ImGui::Separator();

        if (ImGui::CollapsingHeader("预设管理 (Preset Management)"))
        {
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

        ImGui::Text("状态: %s", statusMessage.c_str());


        ImGui::End(); 

        if (needsViewResetOnGenerate)
        {
            fitViewToMap(view, window, dataManager);
            needsViewResetOnGenerate = false; 
        }

		// --- 动画效果 ---
        float sine = sin(animationClock.getElapsedTime().asSeconds() * 0.5f);
        float scale = 1.0f + 0.005f * sine;

        float mapWidthPx = static_cast<float>(dataManager.gridWidth * dataManager.tileSize);
        float mapHeightPx = static_cast<float>(dataManager.gridHeight * dataManager.tileSize);
        tileMap.setOrigin(mapWidthPx / 2.f, mapHeightPx / 2.f);

        tileMap.setPosition(mapWidthPx / 2.f, mapHeightPx / 2.f);
        tileMap.setScale(scale, scale);


        // --- 渲染 ---
        window.clear(sf::Color(50, 50, 50)); 

        /*      背景
        backgroundSprite.setOrigin(backgroundTexture.getSize().x / 2.f, backgroundTexture.getSize().y / 2.f);
        backgroundSprite.setPosition(view.getCenter() * 0.5f);
        window.setView(window.getDefaultView());
        window.draw(backgroundSprite);
        */


        window.setView(view);
        window.draw(tileMap); 
        window.setView(window.getDefaultView()); 
        ImGui::SFML::Render(window); 
        window.display(); 
    }

    ImGui::SFML::Shutdown();

    return 0; 
}


