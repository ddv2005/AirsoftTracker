#include "mapTiles.h"
#include <fstream>
#include <algorithm>
#include "config.h"
#include "atcommon.h"

void sMapTile::clear()
{
    memset(this,0,sizeof(sMapTile));
}

bool sMapTile::loadFromJson(const JsonVariant &json, uint32_t baseAddress)
{
    if(json.containsKey("size")&&json.containsKey("bounds")&&json.containsKey("x")&&json.containsKey("y")
        &&json.containsKey("address")&&json.containsKey("image_stride")&&json.containsKey("image_size")&&json.containsKey("image_fmt"))
    {
        sizeX = json["size"][0];
        sizeY = json["size"][1];
        address = json["address"];
        address += baseAddress;
        size = json["image_size"];
        stride = json["image_stride"];
        fmt = json["image_fmt"];
        tileX = json["x"];
        tileY = json["y"];
        bound.x0 = json["bounds"][0].as<double>();
        bound.y0 = json["bounds"][1].as<double>();
        bound.x1 = json["bounds"][2].as<double>();
        bound.y1 = json["bounds"][3].as<double>();
        mapSizeX = bound.x1-bound.x0;
        mapSizeY = bound.y0-bound.y1;
        return true;
    }
    return false;
}

cMapZoomLevel::cMapZoomLevel()
{
    clear();
}

void cMapZoomLevel::clear()
{
    memset(&m_zlevel,0,sizeof(m_zlevel));
    m_images.clear();
}

void cMapZoomLevel::loadFromBinary(std::ifstream &file)
{
    uint16_t cnt = 0;
    file.read((char*)&m_zlevel,sizeof(m_zlevel));
    file.read((char*)&cnt,2);
    m_images.resize(cnt);
    file.read((char*)m_images.data(),cnt*sizeof(sMapTile));
}

bool cMapZoomLevel::loadFromJson(const JsonVariant &json, uint32_t baseAddress)
{
    if(json.containsKey("pixel_size")&&json.containsKey("bounds")&&json.containsKey("images")&&json.containsKey("tile_size")&&json.containsKey("id"))
    {
        clear();
        m_zlevel.id = json["id"];
        m_zlevel.pixelSizeX = json["pixel_size"][0].as<double>();
        m_zlevel.pixelSizeY = json["pixel_size"][1].as<double>();
        m_zlevel.tileSizeX = json["tile_size"][0].as<double>();
        m_zlevel.tileSizeY = json["tile_size"][1].as<double>();
        m_zlevel.bound.x0 = json["bounds"][0].as<double>();
        m_zlevel.bound.y0 = json["bounds"][1].as<double>();
        m_zlevel.bound.x1 = json["bounds"][2].as<double>();
        m_zlevel.bound.y1 = json["bounds"][3].as<double>();
        const JsonVariant &images = json.getMember("images");
        m_images.resize(images.size());
        for(int i=0; i<images.size(); i++)
        {
            const JsonVariant &imageJson = images.getElement(i);
            m_images[i].loadFromJson(imageJson,baseAddress);
        }
        return true;
    }
    return false;
}

cMapTiles &cMapZoomLevel::getImages()
{
    return m_images;
}

cMap::cMap()
{
    clear();
}

void cMap::clear()
{
    m_map.name[0] = 0;
    m_map.bound.x0 = m_map.bound.x1 = m_map.bound.y0 = m_map.bound.y1 = 0;
    m_map.baseAddress = 0;
    m_zoomLevels.clear();
    m_loaded = false;
}

void cMap::loadHeader(std::ifstream &file,const char *fileName)
{
    m_binaryFileName = fileName;
    m_loaded = false;
    file.read((char*)&m_map,sizeof(sMap));
}

void cMap::loadMapData()
{
    m_loaded = true;
    m_zoomLevels.clear();
    try{
        DEBUG_MSG("Load map data %s\n",m_map.name);
        std::ifstream file(m_binaryFileName,std::ifstream::binary);
        if(file.good())
        {
            file.seekg(m_map.filePosition);
            byte cnt = 0;
            file.read((char*)&cnt,1);
            m_zoomLevels.resize(cnt);
            for(int i=0; i<cnt; i++)
            {
                cMapZoomLevel &zl = m_zoomLevels[i];
                zl.loadFromBinary(file);
            }
            DEBUG_MSG("Loaded Zoom Levels %d\n",m_zoomLevels.size());
        }
        else
            DEBUG_MSG("Load map error\n");
    }catch(...)
    {
        DEBUG_MSG("Can't load map %s\n",m_map.name);
    }
}

bool cMap::loadFromJson(const JsonVariant &json)
{
    if(json.containsKey("name")&&json.containsKey("bounds")&&json.containsKey("tile_matrix")&&json.containsKey("base_address"))
    {
        clear();
        m_loaded = true;
        strncpy(m_map.name,(const char *) json["name"],sizeof(m_map.name)-1);
        m_map.baseAddress = json["base_address"].as<uint32_t>();
        m_map.bound.x0 = json["bounds"][0].as<double>();
        m_map.bound.y0 = json["bounds"][1].as<double>();
        m_map.bound.x1 = json["bounds"][2].as<double>();
        m_map.bound.y1 = json["bounds"][3].as<double>();
        const JsonVariant &zls = json.getMember("tile_matrix");
        for(int i=0; i<zls.size(); i++)
        {
            cMapZoomLevel zl;
            const JsonVariant &zlJson = zls.getElement(i);
            if(zl.loadFromJson(zlJson,m_map.baseAddress))
            {
                m_zoomLevels.push_back(zl);
            }
        }
        return true;
    }
    return false;
}

cMaps::cMaps()
{

}

/*
void cMaps::convertJsonToBinary(const char *filename)
{
    char fileNameBuffer[32];
    snprintf(fileNameBuffer, sizeof(fileNameBuffer),"%ssflash.json",filename);
    std::ifstream file(fileNameBuffer,std::ifstream::binary);
    if(file)
    {
        DEBUG_MSG("Load maps from %s\n",fileNameBuffer);
        DynamicJsonDocument json(128*1024);
        std::ifstream file(fileNameBuffer,std::ifstream::binary);
        DeserializationError error = deserializeJson(json,file);
        if(!error)
        {
            char mapFile[32];
            for(int i=0; i<json.size(); i++)
            {
                const JsonVariant &mapJson = json.getElement(i);
                const JsonVariant &zls = mapJson.getMember("tile_matrix");
                snprintf(mapFile, sizeof(mapFile),"%smap%d.jbin",filename,i);
                std::ofstream file(mapFile,std::ofstream::binary);
                serializeMsgPack(zls,file);
                DEBUG_MSG("Save map %d to %s\n",i, mapFile);
                mapJson["tile_matrix"] = mapFile;
            }            
            snprintf(mapFile, sizeof(mapFile),"%smaps.jbin",filename);
            std::ofstream file(mapFile,std::ofstream::binary);
            serializeMsgPack(json,file);
            remove(fileNameBuffer);
        }
        else
            DEBUG_MSG("Load maps error %s\n",error.c_str());
    }
}
*/

bool cMaps::loadFromBinary(const char *filename)
{
    m_maps.clear();
    try{
        DEBUG_MSG("Load maps from %s\n",filename);
        std::ifstream file(filename,std::ifstream::binary);
        if(file.good())
        {
            byte cnt = 0;
            file.read((char*)&cnt,1);
            if(file)
            {
                m_maps.reserve(cnt);
                for(int i=0; i<cnt; i++)
                {
                    cMap map;
                    map.loadHeader(file,filename);
                    m_maps.push_back(map);
                }
            }
            return true;
        }
        else
            DEBUG_MSG("Load maps error\n");
    }catch(...)
    {
        DEBUG_MSG("Can't load maps from %s\n",filename);
    }
    return false;
}

bool cMaps::loadFromJson(const char *filename)
{
    m_maps.clear();
    try{
        DEBUG_MSG("Load maps from %s\n",filename);
        DynamicJsonDocument json(156*1024);
        std::ifstream file(filename,std::ifstream::binary);
        DeserializationError error = deserializeJson(json,file);
        if(!error)
        {
            for(int i=0; i<json.size(); i++)
            {
                cMap map;
                const JsonVariant &mapJson = json.getElement(i);
                if(map.loadFromJson(mapJson))
                {
                    m_maps.push_back(map);
                }
            }            
            return true;
        }
        else
            DEBUG_MSG("Load maps error %s\n",error.c_str());
    }catch(...)
    {
        DEBUG_MSG("Can't load maps from %s\n",filename);
    }
    return false;
}

cMapProcessor::cMapProcessor(cMapsList &maps,cMapProcessorCallbacks *callbacks):m_maps(maps),m_lastMap(NULL),m_callbacks(callbacks)
{

}

sBoundaries cMapProcessor::getScreenBoundaries(double x, double y, double radius) const
{
    sBoundaries result;
    result.x0 = x-radius;
    result.y0 = y+radius;
    result.x1 = x+radius;
    result.y1 = y-radius;
    return result;
}

double distance(double x1, double y1, double x2, double y2)
{
    return sqrt(pow(x2 - x1, 2) +
                pow(y2 - y1, 2) * 1.0);
}
 
void cMapProcessor::clearUnusedMaps(double x, double y)
{
    for(auto itr=m_maps.begin(); itr!=m_maps.end();)
    {
        if(&(*itr)!=m_lastMap)
        {
            auto b = (*itr).getMap().bound;
            if(distance(x,y,b.x0,b.y0)>=0000)
            {
                itr = m_maps.erase(itr);
                continue;
            }
        }
        itr++;
    }

}

cMapTilesList cMapProcessor::getTilesToRender(double x, double y, double radius)
{
    cMapTilesList result;
    cMap *map = NULL;
    if(m_lastMap)
    {
        if(checkMap(x,y,*m_lastMap))
        {
            map = m_lastMap;
        }
        else
        {
            m_lastMap = NULL;
        }
    }
    if(!map)
    {
        for(auto itr=m_maps.begin(); itr!=m_maps.end(); itr++)
        {
            if(checkMap(x,y,*itr))
            {
                m_lastMap = &*itr;
                map = m_lastMap;
            }
        }
//        if(map)
//            clearUnusedMaps(x,y);
    }
    if(map)
    {
        if(!map->isLoaded())
        {
            map->loadMapData();
            if(m_callbacks)
                m_callbacks->onMapLoad(map);            
        }
        double zoomDiff = 0;
        cMapZoomLevel *zoomLevel = NULL;
        cMapZoomLevels &zls = map->getMapZoomLevels();
        for(auto itr=zls.begin(); itr!=zls.end(); itr++)
        {
            sMapZoomLevel &zl = itr->getZoomLevel();
            double zoomSize = std::max(abs(zl.tileSizeX),abs(zl.tileSizeY))/2;
            if(!zoomLevel || abs(zoomSize-radius)<zoomDiff)
            {
                zoomDiff = abs(zoomSize-radius);
                zoomLevel = &*itr;
            }
        }
        if(zoomLevel)
        {
            cMapTiles &tiles = zoomLevel->getImages();
            sBoundaries sb = getScreenBoundaries(x,y,radius);
            for(auto itr=tiles.begin(); itr !=tiles.end(); itr++)
            {
                if(checkIntersection(sb,itr->bound))
                {
                    result.push_back(*itr);
                }
            }
        }
    }
    return result;
}

bool cMapProcessor::checkMap(const double &x, const double &y,cMap &map)
{
    return isPointInside(x,y,map.getMap().bound);
}

bool cMapProcessor::checkIntersection(const sBoundaries &b1, const sBoundaries &b2) const
{
    return isPointInside(b1.x0,b1.y0,b2) || isPointInside(b1.x1,b1.y0,b2) || isPointInside(b1.x0,b1.y1,b2) || isPointInside(b1.x1,b1.y1,b2) ||
           isPointInside(b2.x0,b2.y0,b1) || isPointInside(b2.x1,b2.y0,b1) || isPointInside(b2.x0,b2.y1,b1) || isPointInside(b2.x1,b2.y1,b1);
}

void cMapProcessor::calcTile(double x, double y, const sMapTile &tile, double radius, double heading, double &scaleX, double &scaleY, double &tx, double &ty)
{
    scaleX = (tile.mapSizeX)/(radius*2);
    scaleY = (tile.mapSizeY)/(radius*2);
    tx = (tile.bound.x0-x)*tile.sizeX/tile.mapSizeX;//*(240/(240+3/scaleX));
    ty = (y-tile.bound.y0)*tile.sizeY/tile.mapSizeY;//*(240/(240+3/scaleY));
}

double cMapProcessor::calcDistanceC(double x, double y)
{
    double lat1,lng1;
    double lat2,lng2;
    convertMeterToDegree(x,y,lng1,lat1);
    convertMeterToDegree(x,y+1000,lng2,lat2);
    double d = distanceBetween(lat1, lng1, lat2, lng2);
    if(d>0)
        return abs(1000/d);
    else
        return 1;
}