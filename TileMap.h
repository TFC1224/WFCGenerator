#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "WFCGenerator.h" // ��ҪCell�Ķ���

class TileMap : public sf::Drawable, public sf::Transformable
{
public:
    // ���ز�������ͼ�Ķ�������
    bool load(const std::string& tilesetPath, sf::Vector2u tileSize, const std::vector<std::vector<Cell*>>& gridData);

private:
    // ����SFML��draw����������SFML��Ⱦ��ϵ�ĺ���
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    sf::VertexArray m_vertices; // �洢������Ƭ�Ķ���
    sf::Texture m_tileset;      // �洢��Ƭ������
};