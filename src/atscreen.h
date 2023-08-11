#pragma once
#include <Arduino.h>
#include "atglobal.h"
#include "utils.h"
#include "atcommon.h"

struct sColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    sColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a=255):r(_r),g(_g),b(_b),a(_a)
    {
    }


    sColor(uint32_t c):r((c >> 16)&0xFF),g((c >> 8)&0xFF),b(c&0xFF),a((c >> 24)&0xFF)
    {
    }

    sColor operator+(const sColor& c)
    {
        sColor cc(std::min(255,(uint16_t)r+c.r),std::min(255,(uint16_t)g+c.g),std::min(255,(uint16_t)b+c.b),std::min(255,(uint16_t)a+c.a));
        return cc;
    }

    uint8_t getA()
    {
        return a;
    }

    uint32_t getRGB()
    {
        return (((uint32_t)r) << 16) | (((uint32_t)g) << 8) | b; 
    }

    uint32_t getRBG()
    {
        return (((uint32_t)r) << 16) | (((uint32_t)b) << 8) | g; 
    }

    uint32_t getEVEColor()
    {
        return ((4UL<<24)|(((r)&255UL)<<16)|(((g)&255UL)<<8)|b); 
    }

    uint32_t getEVEColorA()
    {
        return ((0x10UL<<24)|a); 
    }


} __attribute__ ((packed));

typedef sColor  color_t;
struct sScreenTheme
{
    color_t bgColor;
    color_t textColor;
    color_t bootTextColor;
    color_t circleOuterColor;
    color_t circleLineColor;
    color_t circleBackgroundColor;
    color_t circleHeadingColor;
    color_t peerTextColor;
    color_t peerNormalColor;
    color_t peerDeadColor;
    color_t peerEnemyColor;
    color_t sideButtonsTextColor;
    color_t timerTextColor;
    color_t timerRectColor;
    color_t peerStaticColor;
    color_t peerDynamicColor;
    color_t peerAccuracyMask;
    uint8_t peerAccuracyA;

    sScreenTheme():bgColor(0x0),textColor(0x208020),bootTextColor(255,255,255),circleOuterColor(0x404040),circleLineColor(8,23,153),circleBackgroundColor(89-40, 150-40, 145-40),circleHeadingColor(255,0,0),
        peerTextColor(255,255,255),peerNormalColor(0,255,0),peerDeadColor(255,0,0),peerEnemyColor(0xff4dff),sideButtonsTextColor(0x60A060),timerTextColor(255,0,0),timerRectColor(128,128,128),peerStaticColor(0,0,255),
        peerDynamicColor(0,0,255),peerAccuracyMask(64,64,64),peerAccuracyA(64)
    {
    }
};

// Screen state
#define SS_BOOT     1
#define SS_MAIN     2
#define SS_MENU     3

#define EV_BTN_ON   1
#define EV_BTN_OFF  2
class cATScreen
{
protected:
    atGlobal &m_global;    
    sScreenTheme m_theme;
    uint8_t m_screenState;    
public:
    cATScreen(atGlobal &global);
    virtual ~cATScreen();

    virtual bool init();
    virtual void showBootScreen(const char * msg1,const char * msg2)=0;
    virtual void showShutdownScreen() {};
    virtual void closeBootScreen()=0;
    virtual void beginMainScreen()=0;
    virtual void drawPOI(sPOI &poi,int16_t count=0)=0;
    virtual void endMainScreen()=0;
    virtual void setBacklight(uint8_t level)=0;
    virtual void screenOn()=0;
    virtual void screenOff()=0;
    virtual void drawSideButtons()=0;
    virtual void showProgramMode()=0;
    virtual void showMenu(){};
    virtual uint32_t getFramesCount()=0;
    virtual void loop() {};
    virtual uint8_t getScreenState() { return m_screenState; }
    virtual void setScreenState(uint8_t value) { m_screenState=value; } 
    virtual void processEvent(uint8_t event, uint32_t param1, uint32_t param2){};
    virtual void drawTimers(bool forceAll) {};
    virtual uint16_t getRadius() = 0;

    //Flash interface
    virtual uint32_t flashProgrammStart()=0;
    virtual void flashProgrammEnd()=0;
    virtual void flashWrite(uint32_t address,uint16_t size, uint8_t *data)=0;
    virtual void flashRead(uint32_t address,uint16_t size, uint8_t *data)=0;
};
