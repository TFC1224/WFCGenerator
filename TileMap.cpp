#include "TileMap.h"

bool TileMap::load(const std::string& tilesetPath, sf::Vector2u tileSize, const std::vector<std::vector<Cell*>>& gridData)
{
    // 1. 加载瓦片集纹理
    if (!m_tileset.loadFromFile(tilesetPath))
        return false;

    // 获取网格的宽度和高度
    size_t width = gridData.empty() ? 0 : gridData[0].size();
    size_t height = gridData.size();

    // 2. 准备顶点数组
    // 使用Quads（四边形）类型，每个瓦片需要4个顶点
    m_vertices.setPrimitiveType(sf::Quads);
    m_vertices.resize(width * height * 4);

    // 3. 遍历网格数据，填充顶点数组
    for (unsigned int y = 0; y < height; ++y)
    {
        for (unsigned int x = 0; x < width; ++x)
        {
            // 获取当前单元格的数据
            Cell* cell = gridData[y][x];
            if (!cell || !cell->isCollapsed) continue; // 如果单元格不存在或未坍缩，则跳过

            // 获取该模块在瓦片集上的贴图坐标
            int tileX = cell->module->tileIndex.x; // Module类现在有tileIndex成员
            int tileY = cell->module->tileIndex.y;

            // 计算当前瓦片在顶点数组中的索引
            sf::Vertex* quad = &m_vertices[(x + y * width) * 4];

            // 定义瓦片在窗口中的四个角的位置
            quad[0].position = sf::Vector2f(x * tileSize.x, y * tileSize.y);
            quad[1].position = sf::Vector2f((x + 1) * tileSize.x, y * tileSize.y);
            quad[2].position = sf::Vector2f((x + 1) * tileSize.x, (y + 1) * tileSize.y);
            quad[3].position = sf::Vector2f(x * tileSize.x, (y + 1) * tileSize.y);

            // 定义瓦片在瓦片集贴图上对应的四个角的坐标
            quad[0].texCoords = sf::Vector2f(tileX * tileSize.x, tileY * tileSize.y);
            quad[1].texCoords = sf::Vector2f((tileX + 1) * tileSize.x, tileY * tileSize.y);
            quad[2].texCoords = sf::Vector2f((tileX + 1) * tileSize.x, (tileY + 1) * tileSize.y);
            quad[3].texCoords = sf::Vector2f(tileX * tileSize.x, (tileY + 1) * tileSize.y);
        }
    }

    return true;
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    // 应用变换（例如移动、旋转、缩放地图）
    states.transform *= getTransform();
    // 应用瓦片集纹理
    states.texture = &m_tileset;
    // 绘制顶点数组
    target.draw(m_vertices, states);
}