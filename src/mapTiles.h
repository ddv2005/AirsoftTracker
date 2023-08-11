#pragma once
#define ARDUINOJSON_USE_DOUBLE 1
#include <Arduino.h>
#include <ArduinoJson.h>
#include <stdint.h>
#include <list>
#include <map>
#include <vector>

#pragma pack(push, 1)
//16
struct sBoundaries
{
    float x0;
    float y0;
    float x1;
    float y1;
};

//44
struct sMapTile
{
    uint32_t address;
    uint32_t size;
    uint16_t sizeX;
    uint16_t sizeY;
    uint16_t stride;
    uint16_t fmt;
    uint16_t tileX;
    uint16_t tileY;
    float mapSizeX;
    float mapSizeY;
    sBoundaries bound;
    sMapTile() { clear(); }
    void clear();
    bool loadFromJson(const JsonVariant &json, uint32_t baseAddress);
};

//34
struct sMapZoomLevel
{
    uint16_t id;
    sBoundaries bound;
    float   pixelSizeX;
    float   pixelSizeY;
    float   tileSizeX;
    float   tileSizeY;
};

typedef std::vector<sMapTile> cMapTiles;
typedef std::list<sMapTile> cMapTilesList;
class cMapZoomLevel
{
protected:
    sMapZoomLevel m_zlevel;
    cMapTiles m_images;
public:
    cMapZoomLevel();

    sMapZoomLevel &getZoomLevel()
    {
        return m_zlevel;
    }
    void clear();
    bool loadFromJson(const JsonVariant &json, uint32_t baseAddress);
    void loadFromBinary(std::ifstream &file);
    cMapTiles &getImages(); 
};

//60
struct sMap
{
    char name[32];
    sBoundaries bound;
    uint32_t baseAddress;
    uint32_t filePosition;
    uint32_t size;
};

typedef std::vector<cMapZoomLevel> cMapZoomLevels;
class cMap
{
protected:
    sMap m_map;
    cMapZoomLevels m_zoomLevels;
    bool m_loaded;
    std::string m_binaryFileName;
public:
    cMap();
    sMap &getMap()
    {
        return m_map;
    }
    void clear();
    bool loadFromJson(const JsonVariant &json);
    cMapZoomLevels &getMapZoomLevels()
    {
        return m_zoomLevels;
    }
    void loadHeader(std::ifstream &file,const char *fileName);
    void loadMapData();
    bool isLoaded() { return m_loaded; }
};


typedef std::vector<cMap> cMapsList;
class cMaps
{
protected:
    cMapsList m_maps;
public:    
    cMaps();
    cMapsList &getMaps()
    {
        return m_maps;
    }
    bool loadFromJson(const char *filename);
    bool loadFromBinary(const char *filename);
    bool loadMapData(sMap *m_map);
};

class cMapProcessorCallbacks
{
public:
    virtual void onMapLoad(cMap *map)=0;
};

class cMapProcessor
{
protected:
    cMapsList &m_maps;
    cMap *m_lastMap;
    cMapProcessorCallbacks *m_callbacks;
    bool checkMap(const double &x, const double &y,cMap &map);
    void clearUnusedMaps(double x, double y);
public:
    cMapProcessor(cMapsList &maps,cMapProcessorCallbacks *callbacks);
    sBoundaries getScreenBoundaries(double x, double y, double radius) const;
    cMapTilesList getTilesToRender(double x, double y, double radius);
    bool checkIntersection(const sBoundaries &b1, const sBoundaries &b2) const;
    bool isPointInside(const double &x, const double &y,const sBoundaries &b) const
    {
        return (x>=b.x0 && x<=b.x1 && y>=b.y1 && y<=b.y0);
    }

    double calcDistanceC(double x, double y);
    void calcTile(double x, double y, const sMapTile &tile, double radius, double heading, double &scaleX, double &scaleY, double &tx, double &ty);
};

#pragma pack(pop)