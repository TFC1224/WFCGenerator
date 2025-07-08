#include "TileMap.h"

bool TileMap::load(const std::string& tilesetPath, sf::Vector2u tileSize, const std::vector<std::vector<Cell*>>& gridData)
{
    // 1. ������Ƭ������
    if (!m_tileset.loadFromFile(tilesetPath))
        return false;

    // ��ȡ����Ŀ�Ⱥ͸߶�
    size_t width = gridData.empty() ? 0 : gridData[0].size();
    size_t height = gridData.size();

    // 2. ׼����������
    // ʹ��Quads���ı��Σ����ͣ�ÿ����Ƭ��Ҫ4������
    m_vertices.setPrimitiveType(sf::Quads);
    m_vertices.resize(width * height * 4);

    // 3. �����������ݣ���䶥������
    for (unsigned int y = 0; y < height; ++y)
    {
        for (unsigned int x = 0; x < width; ++x)
        {
            // ��ȡ��ǰ��Ԫ�������
            Cell* cell = gridData[y][x];
            if (!cell || !cell->isCollapsed) continue; // �����Ԫ�񲻴��ڻ�δ̮����������

            // ��ȡ��ģ������Ƭ���ϵ���ͼ����
            int tileX = cell->module->tileIndex.x; // Module��������tileIndex��Ա
            int tileY = cell->module->tileIndex.y;

            // ���㵱ǰ��Ƭ�ڶ��������е�����
            sf::Vertex* quad = &m_vertices[(x + y * width) * 4];

            // ������Ƭ�ڴ����е��ĸ��ǵ�λ��
            quad[0].position = sf::Vector2f(x * tileSize.x, y * tileSize.y);
            quad[1].position = sf::Vector2f((x + 1) * tileSize.x, y * tileSize.y);
            quad[2].position = sf::Vector2f((x + 1) * tileSize.x, (y + 1) * tileSize.y);
            quad[3].position = sf::Vector2f(x * tileSize.x, (y + 1) * tileSize.y);

            // ������Ƭ����Ƭ����ͼ�϶�Ӧ���ĸ��ǵ�����
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
    // Ӧ�ñ任�������ƶ�����ת�����ŵ�ͼ��
    states.transform *= getTransform();
    // Ӧ����Ƭ������
    states.texture = &m_tileset;
    // ���ƶ�������
    target.draw(m_vertices, states);
}