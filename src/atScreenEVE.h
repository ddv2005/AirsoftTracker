#pragma once
#include <stdint.h>
#include "atScreenLVGL.h"
extern "C"
{
    #include <lvgl_tft/EVE_commands.h>
}

bool EVE_loadImage(const char* filename, uint32_t ptr, uint32_t options);    
bool EVE_inflateImage(const char* filename, uint32_t ptr);

struct sEVEImage
{
    uint32_t ptr;
    uint16_t w;
    uint16_t h;
    uint16_t fmt;
    sEVEImage()
    {
        ptr = 0;
        w = h = fmt = 0;
    }

    uint32_t load(const char * filename,uint32_t _ptr)
    {
        uint32_t result=0;
        uint32_t _w, _h;
        ptr = _ptr;
        EVE_loadImage(filename,_ptr,EVE_OPT_NODL);
        EVE_LIB_GetProps(&result,&_w,&_h);
        fmt = EVE_ARGB4;
        w = _w;
        h = _h;
        return result;
    }

    void setImage()
    {
        EVE_cmd_setbitmap(ptr,fmt, w, h);            
    }
}__attribute__((packed));

struct sMapsMemoryItem
{
    uint32_t address;
    uint32_t memoryaddress;
    uint32_t size;
    int64_t  lastUsed;
    uint32_t usageCounter;
    uint8_t  inUse:1;
    uint8_t  inCurrentList:1;
};

#define MAX_MAP_IMAGE_SIZE 59536
#define MAPS_MEMORY_ITEMS  9
#define MAPS_MEMORY_BASE   (EVE_RAM_G_SIZE-(MAPS_MEMORY_ITEMS*MAX_MAP_IMAGE_SIZE))
class cMapsMemory
{
protected:
    sMapsMemoryItem *m_memory;
    uint8_t m_count;
    int8_t findItem(const sMapTile &tile);
public:
    cMapsMemory(uint8_t count);
    ~cMapsMemory();

    void clearCurrent();
    void markCurrent(const sMapTile &tile);
    bool allocItem(const sMapTile &tile,int64_t now, uint32_t &address, bool &needLoad);
};


struct sLabelsRect
{
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;

    int32_t getArea()
    {
        return abs((x2-x1)*(y2-y1));
    }
};

class cATScreenEVE:public cATScreenLVGL
{
protected:
    sEVEImage m_gpsImage;
    uint32_t m_pointer;
    uint8_t  m_backlight;
    double m_heading;
    cMapsMemory m_mapsMemory;
    std::list<sLabelsRect> m_labels;
    double m_distanceC = 1.0;

    void loadAllImages();
    void beginDraw();
    void endDraw();
    void drawBattery();
    void drawGPS();
    void drawDateTime();
    void drawInfo();
    int32_t getMaxLabelOverlap(sLabelsRect r);
public:
    cATScreenEVE(atGlobal &global);
    virtual ~cATScreenEVE();

    virtual bool init();
    virtual void showBootScreen(const char * msg1,const char * msg2);
    virtual void closeBootScreen();
    virtual void beginMainScreen();
    virtual void drawPOI(sPOI &poi,int16_t count=0);
    virtual void endMainScreen();
    virtual void setBacklight(uint8_t level);
    virtual void screenOn();
    virtual void screenOff();
    virtual void drawSideButtons();
    virtual void showProgramMode();
    virtual uint32_t getFramesCount();
    virtual void showMenu();
    virtual void loop();
    virtual void setScreenState(uint8_t value);
    virtual void showShutdownScreen();
    virtual void drawTimers(bool forceAll);
    virtual uint16_t getRadius();


    virtual uint32_t flashProgrammStart();
    virtual void  flashProgrammEnd();
    virtual void flashWrite(uint32_t address,uint16_t size, uint8_t *data);
    virtual void flashRead(uint32_t address,uint16_t size, uint8_t *data);
};
