#pragma once
#include "atscreen.h"

class cATScreenDummy: public cATScreen
{
public:
    cATScreenDummy(atGlobal &global):cATScreen(global) {};
    virtual ~cATScreenDummy(){};

    virtual bool init() { return true; };
    virtual void showBootScreen(const char * msg1,const char * msg2) {};
    virtual void closeBootScreen() {};
    virtual void beginMainScreen() {};
    virtual void drawPOI(sPOI &poi,int16_t count=0) {};
    virtual void endMainScreen() {};
    virtual void setBacklight(uint8_t level) {};
    virtual void screenOn() {};
    virtual void screenOff() {};
    virtual void drawSideButtons() {};
    virtual void showProgramMode() {};
    virtual uint32_t getFramesCount() { return 0;};
    virtual uint16_t getRadius() { return m_global.getConfig().radius; }

    //Flash interface
    virtual uint32_t flashProgrammStart()  { return 0;};
    virtual void flashProgrammEnd()  {};
    virtual void flashWrite(uint32_t address,uint16_t size, uint8_t *data) {};
    virtual void flashRead(uint32_t address,uint16_t size, uint8_t *data) {};
};
