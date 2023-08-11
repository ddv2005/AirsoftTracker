#include "atglobal.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <fstream>


struct PSRAMAllocator {
  void* allocate(size_t size) {
    return heap_caps_malloc(size,MALLOC_CAP_SPIRAM);
  }

  void deallocate(void* ptr) {
    heap_caps_free(ptr);
  }

  void* reallocate(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size,MALLOC_CAP_SPIRAM);
  }
};

typedef BasicJsonDocument<PSRAMAllocator> PSRAMJsonDocument;

void atGlobal::onMapLoad(cMap *map)
{
    m_poiMapStatic.clear();
    if(map)
    {
        char fileName[32];
        std::string mapName = map->getMap().name;
        for(auto& c : mapName)
        {
            c = tolower(c);
        }
        snprintf(fileName,sizeof(fileName),AT_POINTS_FILE_TEMPLATE,mapName.c_str());
        std::ifstream file(fileName,std::ifstream::binary);
        if(file.good())
        {
            DEBUG_MSG("Loading points from %s\n",fileName);            
            PSRAMJsonDocument json(64*1024);
            DeserializationError error = deserializeJson(json,file);
            if(!error)
            {
                const JsonVariant &points = json.getMember("points");
                for(int i=0; i<points.size(); i++)
                {
                    sPOI poi;
                    const JsonVariant &poiJson = points.getElement(i);
                    if(poi.loadFromJson(poiJson))
                    {
                        poi.updateXY();
                        m_poiMapStatic.push_back(poi);
                    }
                }
            }
            else
                DEBUG_MSG("Load maps error %s\n",error.c_str());
        }
    }
}

bool sPOI::loadFromJson(const JsonVariant &json)
{
    if(json.containsKey("type")&&json.containsKey("color")&&json.containsKey("name")&&json.containsKey("coordinates"))
    {
        name = (const char *) json["name"];
        uint8_t _t = json["type"].as<uint8_t>();
        switch(_t==0)
        {
            case 0:
                type = POIT_STATIC;
                break;
            default:
                type = POIT_STATIC;
                break;
        }
        color = json["color"].as<uint32_t>();
        lat = json["coordinates"][1].as<double>();
        lng = json["coordinates"][0].as<double>();
        positionChanged = true;
        return true;
    }
    return false;
}

cConfig::cConfig()
{
}

bool cConfig::loadConfig(std::istream &stream,bool binary)
{
    try{
        StaticJsonDocument<1024> json;
        DeserializationError error;
        if(binary)
            error = deserializeMsgPack(json,stream);
        else
            error = deserializeJson(json,stream);
        if(error)
        {
            DEBUG_MSG("cConfig loading error %s\n",error.c_str());
            return false;
        }
        else
        {
            initDefaults();
            loadConfigValues(json);
        }
    }catch(...)
    {
        return false;
    }
    return true;
}

void cConfig::saveConfig(std::ostream &stream,bool binary)
{
    StaticJsonDocument<1024> json;
    saveConfigValues(json);

    try{
        if(binary)
            serializeMsgPack(json,stream);
        else
            serializeJson(json,stream);
    }catch(...)
    {
    }
}


void cConfig::loadConfig(const char * filename)
{
    try{
        std::ifstream file(filename,std::ifstream::binary);
        if(!loadConfig(file))
            saveConfig(filename);
    }catch(...)
    {
        saveConfig(filename);
    }
    if(checkConfig())
        saveConfig(filename);
}

void cConfig::saveConfig(const char * filename)
{
    try{
        std::ofstream file(filename,std::ofstream::binary);
        saveConfig(file);
    }catch(...)
    {
    }
}


atMainConfig::atMainConfig()
{
    initDefaults();    
}

void atMainConfig::initDefaults()
{
    symbol = 0;
    id = MAX_CONFIG_ID;
    radius = 250;
    units = U_METRIC;
    reportInterval = 500;
    timeZone = DEFAULT_TZ;
    screenIdleTimeout = 90000;
    useVibration = true;
    vibrationTime = 200;
    maxPeerTimeout = 600;
    memset(&timers,0,sizeof(timers));
    checkConfig();
}

bool atMainConfig::checkConfig()
{
    bool changed = false;
    if(id==MAX_CONFIG_ID)
    {
        uint8_t mac[6];
        changed = true;
        esp_read_mac(mac, ESP_MAC_BT);
        id = mac[5];
        if(id==0)
            id = rand() & 0xFF;
    }
    if(name.empty())
    {
        changed = true;
        char buffer[16];
        snprintf(buffer,sizeof(buffer),"Dev%02X",(int)id);
        name = buffer;
    }
    if(reportInterval<200)
    {
        changed = true;
        reportInterval = 500;
    }
    if(reportInterval>30000)
    {
        changed = true;
        reportInterval = 30000;
    }
    if(timeZone.empty())
    {
        changed = true;
        timeZone = DEFAULT_TZ;
    }

    if(screenIdleTimeout<10000 && screenIdleTimeout!=0)
    {
        screenIdleTimeout=10000;
        changed = true;
    }

    if(vibrationTime<50)
    {
        vibrationTime = 50;
        changed = true;
    }
    else
        if(vibrationTime>2000)
        {
            vibrationTime = 2000;
            changed = true;
        }

    if(maxPeerTimeout>86400)
    {
        maxPeerTimeout = 86400;
        changed = true;
    }
    return changed;
}

void atMainConfig::loadConfigValues(JsonDocument &json)
{
    const char * c = (const char*) json["tz"];
    testLat = json["testLat"];
    testLng = json["testLng"];
    id = json["id"];
    radius = json["radius"];
    symbol = json["symbol"];
    reportInterval = json["reportInterval"];
    screenIdleTimeout = json["screenIdleTimeout"];
    for(int i=0; i<AT_MAX_TIMERS; i++)
    {
        char buf[4];
        snprintf(buf,sizeof(buf),"t%d",i+1);
        timers[i] = json[buf];
    }
    if(json.containsKey("useVibration"))
    {
        useVibration = json["useVibration"];
        vibrationTime = json["vibrationTime"];
    }
    if(json.containsKey("maxPeerTimeout"))
    {
        maxPeerTimeout = json["maxPeerTimeout"];
    }
    c = (const char*) json["name"];
    if(c)
        name = c;
    units = json["units"];
    c = (const char*) json["tz"];
    if(c)
        timeZone = c;
}

void atMainConfig::saveConfigValues(JsonDocument &json)
{
    json["id"] = id;
    json["symbol"] = symbol;
    json["radius"] = radius;
    json["name"] = name.c_str();
    json["units"] = units;
    json["tz"] = timeZone.c_str();
    json["screenIdleTimeout"] = screenIdleTimeout;
    json["reportInterval"] = reportInterval;
    json["useVibration"] = useVibration;
    json["vibrationTime"] = vibrationTime;
    json["maxPeerTimeout"] = maxPeerTimeout;
    for(int i=0; i<AT_MAX_TIMERS; i++)
    {
        char buf[4];
        snprintf(buf,sizeof(buf),"t%d",i+1);
        json[buf] = timers[i];
    }
}

////////////////////////////////////////
void atNetworkConfig::initDefaults()
{
    channelConfig = cLoraChannelCondfig();
    channelConfig.bw = LORA_BW;
    channelConfig.sf = LORA_SF;
    channelConfig.cr = LORA_CR;
    channelConfig.freq = LORA_FREQ;
    channelConfig.power = LORA_POWER;
    channelConfig.preambleLength = LORA_PRE;
    channelConfig.syncWord = LORA_SYNC;
    channelConfig.currentLimit = LORA_CURRENT;

    password = "";
    networkId = 0;

    requestInfoInterval = 10000;
}

bool atNetworkConfig::checkConfig()
{
    return false;
}

void atNetworkConfig::saveConfigValues(JsonDocument &json)
{
    json["freq"] = channelConfig.freq;
    json["bw"] = channelConfig.bw;
    json["sf"] = channelConfig.sf;
    json["cr"] = channelConfig.cr;
    json["preambleLength"] = channelConfig.preambleLength;
    json["syncWord"] = channelConfig.syncWord;
    json["networkId"] = networkId;
    json["password"] = password.c_str();
}

void atNetworkConfig::loadConfigValues(JsonDocument &json)
{
    channelConfig.freq = json["freq"];
    channelConfig.bw = json["bw"];
    channelConfig.sf = json["sf"];
    channelConfig.cr = json["cr"];
    channelConfig.preambleLength = json["preambleLength"];
    channelConfig.syncWord = json["syncWord"];
    networkId = json["networkId"];
    const char *c = json["password"];
    if(c)
        password = c;
}

atNetworkConfig::atNetworkConfig()
{
    initDefaults();
}


////////////////////////////////////////
void atAGDNetworkConfig::initDefaults()
{
    channelConfig = cLoraChannelCondfig();
    channelConfig.bw = AGD_LORA_BW;
    channelConfig.sf = AGD_LORA_SF;
    channelConfig.cr = AGD_LORA_CR;
    channelConfig.freq = AGD_LORA_FREQ;
    channelConfig.power = LORA_POWER;
    channelConfig.preambleLength = AGD_LORA_PRE;
    channelConfig.syncWord = AGD_LORA_SYNC;
    channelConfig.currentLimit = LORA_CURRENT;
    updateTimeout = 10000;
}

bool atAGDNetworkConfig::checkConfig()
{
    bool changed = false;

    if(updateTimeout<6000)
    {
        updateTimeout = 6000;
        changed = true;
    }
    else
    if(updateTimeout>60000)
    {
        updateTimeout = 60000;
        changed = true;
    }

    return changed;
}

void atAGDNetworkConfig::saveConfigValues(JsonDocument &json)
{
    json["freq"] = channelConfig.freq;
    json["bw"] = channelConfig.bw;
    json["sf"] = channelConfig.sf;
    json["cr"] = channelConfig.cr;
    json["preambleLength"] = channelConfig.preambleLength;
    json["syncWord"] = channelConfig.syncWord;
    json["updateTimeout"] = updateTimeout;
}

void atAGDNetworkConfig::loadConfigValues(JsonDocument &json)
{
    channelConfig.freq = json["freq"];
    channelConfig.bw = json["bw"];
    channelConfig.sf = json["sf"];
    channelConfig.cr = json["cr"];
    channelConfig.preambleLength = json["preambleLength"];
    channelConfig.syncWord = json["syncWord"];
    updateTimeout = json["updateTimeout"];
}

atAGDNetworkConfig::atAGDNetworkConfig()
{
    initDefaults();
}

cSideButtons::cSideButtons()
{
    for(int i=0; i<SIDE_BUTTONS_COUNT; i++)
    {
        m_buttons[i].idx = i;
        m_buttons[i].action = 0;
    }
}