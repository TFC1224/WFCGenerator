#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "WFCGenerator.h" // 需要Cell的定义

class TileMap : public sf::Drawable, public sf::Transformable
{
public:
    // 加载并构建地图的顶点数据
    bool load(const std::string& tilesetPath, sf::Vector2u tileSize, const std::vector<std::vector<Cell*>>& gridData);

private:
    // 重载SFML的draw函数，这是SFML渲染体系的核心
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    sf::VertexArray m_vertices; // 存储所有瓦片的顶点
    sf::Texture m_tileset;      // 存储瓦片集纹理
};