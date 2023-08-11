#include <sstream>
#include "config.h"
#ifdef USE_SCREEN_EVE
#include "atScreenEVE.h"
#include "gps/GPS.h"
#include "../include/version.h"
#include <iomanip>

#define EO_BATT 40
#define EO_STAT_Y 14
#define EO_STAT_FONT 20
#define EO_BOOT_FONT 20
#define EO_SIDE_BTN_FONT 26
#define EO_SPACE 5
#define EO_POI_FONT 15
#define EO_TIMERS_FONT 26
#define EO_TIMERS_FONT_SIZE 16


static const uint8_t PROGMEM eveFont14[] = 
#include "calibri_14_L4.rawh"
;


cATScreenEVE::cATScreenEVE(atGlobal &global):cATScreenLVGL(global),m_mapsMemory(MAPS_MEMORY_ITEMS)
{
    m_pointer = 0;
    m_backlight = 0x20;
}

cATScreenEVE::~cATScreenEVE()
{

}

bool  cATScreenEVE::init()
{
    if(cATScreenLVGL::init())
    {
        DEBUG_MSG("EVE frequency %dMhz\n",EVE_memRead32(REG_FREQUENCY)/1000000);
        if(EVE_memRead32(REG_FREQUENCY)==0)
            return false;
        EVE_cmd_flashattach();
        EVE_cmd_flashfast();
        m_pointer = EVE_HSIZE*EVE_VSIZE*2;
        EVE_memWrite_buffer(m_pointer,eveFont14,sizeof(eveFont14),false);
        EVE_cmd_execute();
        m_pointer += sizeof(eveFont14)+32;
        m_pointer = (m_pointer/32)*32;
        EVE_memWrite32(REG_PWM_HZ, 2000);
        setBacklight(100);
        return true;
    }
    else
    {
        return false;
    }
}

void cATScreenEVE::beginDraw()
{
    EVE_cmd_dl(CMD_DLSTART);
}

void cATScreenEVE::endDraw()
{
	EVE_cmd_dl(DL_DISPLAY);
	EVE_cmd_dl(CMD_SWAP);
	EVE_cmd_execute();    
}

void cATScreenEVE::loadAllImages()
{
    m_pointer = m_gpsImage.load("/spiffs/gps.png",EVE_RAM_G+m_pointer);
}

void cATScreenEVE::screenOn()
{
    EVE_cmdWrite(EVE_ACTIVE, 0);
    delay(10);
    EVE_memWrite8(REG_PWM_DUTY, m_backlight);
}

void cATScreenEVE::screenOff()
{
    EVE_memWrite8(REG_PWM_DUTY, 0);
    EVE_cmdWrite(EVE_SLEEP, 0);
}

#define BUILD_YEAR  (__DATE__ + 7)

void cATScreenEVE::showBootScreen(const char * msg1,const char * msg2)
{
    char buffer[64];
    beginDraw();
	EVE_cmd_dl(DL_CLEAR_RGB | 0x000000UL);
	EVE_cmd_dl(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
    EVE_loadImage("/spiffs/at.png",EVE_RAM_G+m_pointer,0);
    EVE_cmd_dl(BEGIN(EVE_BITMAPS));
    EVE_cmd_dl(VERTEX2II(0, 0, 0, 0));
    EVE_cmd_dl(COLOR_RGB(m_theme.bootTextColor.r,m_theme.bootTextColor.g,m_theme.bootTextColor.b));
    snprintf(buffer,sizeof(buffer),"Version %s (%s), Copyright DDV %s",VERSION,__DATE__,BUILD_YEAR);
    EVE_cmd_text(EVE_HSIZE,EVE_VSIZE-EO_BOOT_FONT-2,EO_BOOT_FONT,EVE_OPT_RIGHTX,buffer);
    if(msg1)
    {
        EVE_cmd_text(EVE_HSIZE/2,EVE_VSIZE-EO_BOOT_FONT*3-2,EO_BOOT_FONT,EVE_OPT_CENTERX,msg1);
    }
    if(msg2)
    {
        EVE_cmd_text(EVE_HSIZE/2,EVE_VSIZE-EO_BOOT_FONT*1.5-2,EO_BOOT_FONT,EVE_OPT_CENTERX,msg2);
    }
    EVE_cmd_spinner(EVE_HSIZE/2,EVE_VSIZE/2,0,0);
    endDraw();
}

void cATScreenEVE::closeBootScreen()
{
	beginDraw();
	EVE_cmd_dl(DL_CLEAR_RGB | 0x000000UL);
	EVE_cmd_dl(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
	endDraw();
    loadAllImages();
}

void cATScreenEVE::drawInfo()
{
    char buffer[32];
    snprintf(buffer,sizeof(buffer),"ID #%d, %s",(int)m_global.getConfig().id,m_global.getConfig().name.c_str());
    EVE_cmd_text(EVE_HSIZE-EO_SPACE,EO_STAT_Y,EO_STAT_FONT,EVE_OPT_RIGHTX,buffer);
    snprintf(buffer,sizeof(buffer),"Status: %s",m_global.m_playerStatus.status==PS_NORMAL?ATS_ALIVE:ATS_DEAD);
    EVE_cmd_text(EVE_HSIZE-EO_SPACE,EO_STAT_Y*2,EO_STAT_FONT,EVE_OPT_RIGHTX,buffer);
}

void cATScreenEVE::drawDateTime()
{
    char buffer[32];
    if(getValidTime()>0)
    {
        time_t now;
        struct tm timeinfo;        
        time(&now);
        localtime_r(&now, &timeinfo);

        snprintf(buffer,sizeof(buffer),"%02d:%02d:%02d %02d/%02d/%04d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_year+1900);
    }
    else
        snprintf(buffer,sizeof(buffer),"--:--:--");
    EVE_cmd_text(EO_SPACE,0,EO_STAT_FONT,0,buffer);
}

void cATScreenEVE::drawBattery()
{
    char buffer[32];
    snprintf(buffer,sizeof(buffer),"%d%% (%0.2fV)",m_global.m_powerStatus->getBatteryChargePercent(),m_global.m_powerStatus->getBatteryVoltageMv()/1000.0);
    EVE_cmd_text(EVE_HSIZE-EO_BATT,0,EO_STAT_FONT,EVE_OPT_RIGHTX,buffer);
    //snprintf(buffer,sizeof(buffer),"%0.0f mA /%0.0f / %d",m_global.getPower()->getAXP().isChargeing()?m_global.getPower()->getAXP().getBattChargeCurrent():m_global.getPower()->getAXP().getBattDischargeCurrent(),m_global.getPower()->getAXP().getCoulombData(),m_backlight);
    //EVE_cmd_text(EVE_HSIZE-EO_SPACE,EVE_VSIZE-EO_STAT_Y,EO_STAT_FONT,EVE_OPT_RIGHTX,buffer);
}

void cATScreenEVE::drawGPS()
{
    char buffer[32];
    snprintf(buffer,sizeof(buffer),"%d",m_global.m_gpsStatus->getNumSatellites());
    EVE_cmd_text(EVE_HSIZE-EO_BATT+EO_SPACE*3+m_gpsImage.w,0,EO_STAT_FONT,0,buffer);
    m_gpsImage.setImage();
    EVE_cmd_dl(BEGIN(EVE_BITMAPS));
    EVE_cmd_dl(VERTEX2II(EVE_HSIZE-EO_BATT+EO_SPACE*2, 0, 0, 0));
    if(m_global.m_gpsStatus->getHasLock())
    {
        double lat;
        double lng;
        double x = m_global.m_mapCenterX==0?m_global.m_gpsStatus->getX():m_global.m_mapCenterX;
        double y = m_global.m_mapCenterY==0?m_global.m_gpsStatus->getY():m_global.m_mapCenterY;
        convertMeterToDegree(x,y,lng,lat);

        snprintf(buffer,sizeof(buffer),"%010.7f",lat);
        EVE_cmd_text(EO_SPACE,EO_STAT_Y,EO_STAT_FONT,0,buffer);
        snprintf(buffer,sizeof(buffer),"%010.7f",lng);
        EVE_cmd_text(EO_SPACE,EO_STAT_Y*2,EO_STAT_FONT,0,buffer);
    }
}


void cATScreenEVE::setBacklight(uint8_t level)
{
    m_backlight = level/2+1;
    EVE_memWrite8(REG_PWM_DUTY, m_backlight);
}

inline static void calcCirclePoint(double angle, int32_t radius, int32_t &x,int32_t &y)
{
    if(angle<0)
        angle += 2*PI;
    else
    if(angle>2*PI)
        angle -= 2*PI;
    x = radius*cos(angle);
    y = radius*sin(angle);
}

#define R1  (EVE_VSIZE/2)
#define HEADING_R   (EVE_VSIZE/2-12)
//#define HEADING_A   (10*PI/180)
#define HEADING_A   0
void cATScreenEVE::beginMainScreen()
{
    cMapTilesList tiles;
    double radius = m_global.getConfig().radius;

    double xx = m_global.m_mapCenterX==0?m_global.m_gpsStatus->getX():m_global.m_mapCenterX;
    double yy = m_global.m_mapCenterY==0?m_global.m_gpsStatus->getY():m_global.m_mapCenterY;

    if(m_global.m_gpsStatus->getHasLock())
    {
        m_distanceC = m_global.m_mapsProcessor.calcDistanceC(xx,yy);
        radius *= m_distanceC;
        tiles = m_global.m_mapsProcessor.getTilesToRender(xx,yy,radius);
    }
    else
        m_distanceC = 1.0;
    m_heading = (18000-m_global.m_imuStatus->getHeading()+m_global.m_gpsStatus->getMagDec()*100)*PI/18000.0;
    int32_t cx = EVE_HSIZE/2-1;
    int32_t cy = EVE_VSIZE/2-1;
    beginDraw();
	EVE_cmd_dl(DL_CLEAR_RGB | m_theme.bgColor.getRGB());
	EVE_cmd_dl(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG); 

    EVE_cmd_dl(STENCIL_OP(EVE_REPLACE,EVE_REPLACE));
    EVE_cmd_dl(STENCIL_FUNC(EVE_ALWAYS,1,255));    
    EVE_cmd_dl(BEGIN(EVE_POINTS));
    EVE_cmd_dl(POINT_SIZE(R1*16));
    EVE_cmd_dl(m_theme.circleOuterColor.getEVEColor());
    EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));

    EVE_cmd_dl(POINT_SIZE(1888));
    EVE_cmd_dl(m_theme.circleBackgroundColor.getEVEColor());
    EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));

    EVE_cmd_dl(POINT_SIZE(960));
    EVE_cmd_dl(m_theme.circleLineColor.getEVEColor());
    EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));

    EVE_cmd_dl(POINT_SIZE(928));
    EVE_cmd_dl(m_theme.circleBackgroundColor.getEVEColor());
    EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));
    EVE_cmd_dl(END());
    EVE_cmd_dl(STENCIL_OP(EVE_KEEP,EVE_KEEP));

    bool hasTiles = tiles.size();
    if(hasTiles)
    {
        EVE_cmd_dl(SAVE_CONTEXT());
        EVE_cmd_dl(STENCIL_FUNC(EVE_EQUAL,1,1));
        EVE_cmd_dl(ALPHA_FUNC(EVE_GEQUAL, 198));
        EVE_cmd_dl(COLOR_A(255));
        EVE_cmd_dl(COLOR_RGB(255,255,255));
        m_mapsMemory.clearCurrent();
        for(auto itr=tiles.begin();itr!=tiles.end();itr++)
            m_mapsMemory.markCurrent(*itr);

        int64_t now=millis();
        double x = xx-radius;
        double y = yy+radius;
        int ii=0;
        for(auto itr=tiles.begin();itr!=tiles.end();itr++)
        {
            ii++;
            uint32_t address;
            bool needLoad;
            sMapTile &tile=*itr;
            if(m_mapsMemory.allocItem(tile,now,address,needLoad))
            {
                if(needLoad)
                {
                    //DEBUG_MSG("Load %X %X %d\n",MAPS_MEMORY_BASE+address,tile.address+4096,tile.size);
                    EVE_cmd_flashread(MAPS_MEMORY_BASE+address,tile.address+4096,tile.size);
                }
                EVE_cmd_setbitmap(MAPS_MEMORY_BASE+address,tile.fmt,tile.sizeX,tile.sizeY);
                EVE_cmd_dl(BITMAP_SIZE(EVE_BILINEAR,EVE_BORDER,EVE_BORDER,tile.sizeX,tile.sizeY));
                //EVE_cmd_dl(BITMAP_SIZE(EVE_NEAREST,EVE_BORDER,EVE_BORDER,tile.sizeX,tile.sizeY));

                double scaleX,scaleY;
                double tx,ty;
                m_global.m_mapsProcessor.calcTile(x,y,tile,radius,(18000-m_global.m_imuStatus->getHeading())/100,scaleX,scaleY,tx,ty);
                EVE_cmd_dl(CMD_LOADIDENTITY);
                EVE_cmd_scale(65536*scaleX, 65536*scaleY);
                EVE_cmd_translate(7864320/scaleX, 7864320/scaleY);
                EVE_cmd_rotate((m_global.m_gpsStatus->getMagDec()*100-m_global.m_imuStatus->getHeading())/100*182);
                EVE_cmd_translate(-7864320/scaleX, -7864320/scaleY);
                EVE_cmd_translate(65536*tx, 65536*ty);                
                EVE_cmd_dl(CMD_SETMATRIX);

                EVE_cmd_dl(BEGIN(EVE_BITMAPS));
                EVE_cmd_dl(VERTEX2II(40, 0, 0, 0));
            }
        }
        EVE_cmd_dl(RESTORE_CONTEXT());
        EVE_cmd_dl(COLOR_A(255));
    }
    int32_t x,y;
    EVE_cmd_dl(BEGIN(EVE_LINES));
    EVE_cmd_dl(m_theme.circleLineColor.getEVEColor());
    calcCirclePoint(m_heading-PI,R1,x,y);
    EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));
    calcCirclePoint(m_heading,R1,x,y);
    EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));
    EVE_cmd_dl(END());

    EVE_cmd_dl(BEGIN(EVE_LINES));
    calcCirclePoint(m_heading+PI/2,R1,x,y);
    EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));
    calcCirclePoint(m_heading-PI/2,R1,x,y);
    EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));
    EVE_cmd_dl(END());

    EVE_cmd_dl(BEGIN(EVE_POINTS));
    EVE_cmd_dl(POINT_SIZE(64));
    EVE_cmd_dl(m_theme.circleLineColor.getEVEColor());
    EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));

    EVE_cmd_dl(POINT_SIZE(32));
    EVE_cmd_dl(COLOR_RGB(255, 255, 255));
    EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));
    EVE_cmd_dl(END());

    EVE_cmd_dl(m_theme.circleHeadingColor.getEVEColor());
    calcCirclePoint(m_heading-HEADING_A+PI/2,HEADING_R,x,y);
    EVE_cmd_text(x+cx,y+cy,28,EVE_OPT_CENTERX | EVE_OPT_CENTERY,"N");

    calcCirclePoint(m_heading-HEADING_A-PI/2,HEADING_R,x,y);
    EVE_cmd_text(x+cx,y+cy,28,EVE_OPT_CENTERX | EVE_OPT_CENTERY,"S");

    calcCirclePoint(m_heading-HEADING_A-PI,HEADING_R,x,y);
    EVE_cmd_text(x+cx,y+cy,28,EVE_OPT_CENTERX | EVE_OPT_CENTERY,"E");

    calcCirclePoint(m_heading-HEADING_A,HEADING_R,x,y);
    EVE_cmd_text(x+cx,y+cy,28,EVE_OPT_CENTERX | EVE_OPT_CENTERY,"W");

    char buffer[16];
    snprintf(buffer,sizeof(buffer),"%dm",m_global.getConfig().radius);
    EVE_cmd_text(cx+R1+1,cy,26,EVE_OPT_CENTERY,buffer);

    snprintf(buffer,sizeof(buffer),"%03d",(int)m_global.getDirectionDeg());
    EVE_cmd_text(EO_SPACE*2,cy,26,EVE_OPT_CENTERY,buffer);

    if(hasTiles)
    {
//        EVE_cmd_dl(CLEAR_STENCIL(1));
//        EVE_cmd_dl(CLEAR(0,1,0));
        EVE_cmd_dl(STENCIL_OP(EVE_REPLACE,EVE_REPLACE));
        EVE_cmd_dl(STENCIL_MASK(2));
        EVE_cmd_dl(STENCIL_FUNC(EVE_ALWAYS,2,2));
        EVE_cmd_dl(COLOR_A(0));
        
        EVE_cmd_dl(BEGIN(EVE_POINTS));
        EVE_cmd_dl(POINT_SIZE(930));
        EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));

        EVE_cmd_dl(STENCIL_OP(EVE_KEEP,EVE_KEEP));
        EVE_cmd_dl(STENCIL_FUNC(EVE_NOTEQUAL,2,2));
        EVE_cmd_dl(POINT_SIZE(970));
        EVE_cmd_dl(COLOR_A(255));
        EVE_cmd_dl(m_theme.circleLineColor.getEVEColor());
        EVE_cmd_dl(VERTEX2II(cx, cy, 0, 0));
        EVE_cmd_dl(END());
        EVE_cmd_dl(STENCIL_FUNC(EVE_ALWAYS,0,255));
        EVE_cmd_dl(STENCIL_MASK(255));
        EVE_cmd_dl(STENCIL_OP(EVE_KEEP,EVE_KEEP));
    }
    EVE_cmd_setfont2(15,EVE_HSIZE*EVE_VSIZE*2,32);
    EVE_cmd_dl(BITMAP_HANDLE(0));
}

void cATScreenEVE::endMainScreen()
{
    m_labels.clear();
    EVE_cmd_dl(COLOR_RGB(m_theme.textColor.r,m_theme.textColor.g,m_theme.textColor.b));
    drawBattery();
    drawGPS();
    drawDateTime();
    drawInfo();
    drawSideButtons();
    endDraw();
}

int overlappingArea(const sLabelsRect &r1,
                    const sLabelsRect &r2)
{
    // Area of 1st Rectangle
    //int area1 = abs(r1.x1 - r1.x2) * abs(r1.y1 - r1.y2);
 
    // Area of 2nd Rectangle
    //int area2 = abs(r2.x1 - r2.x2) * abs(r2.y1 - r2.y2);
 
    int x_dist = min(r1.x2, r2.x2)
                  - max(r1.x1, r2.x1);
    int y_dist = (min(r1.y2, r2.y2)
                  - max(r1.y1, r2.y1));
    int areaI = 0;
    if( x_dist > 0 && y_dist > 0 )
    {
        areaI = x_dist * y_dist;
    }
     
    return areaI;
}
#define LABEL_SP_X  9
#define LABEL_SP_Y  -6
#define LABEL_MAX_IDX 7

sLabelsRect getLabelPos(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t idx)
{
    sLabelsRect result;
    switch(idx)
    {
        case 1:
        {
            result.x1 = x+LABEL_SP_X;
            result.y1 = y+LABEL_SP_Y-h/2;
            break;
        }

        case 2:
        {
            result.x1 = x+LABEL_SP_X;
            result.y1 = y+LABEL_SP_Y+h/2;
            break;
        }

        case 3:
        {
            result.x1 = x-w/2;
            result.y1 = y+LABEL_SP_Y-h/2;
            break;
        }

        case 4:
        {
            result.x1 = x-w/2;
            result.y1 = y+LABEL_SP_Y+h/2;
            break;
        }

        case 5:
        {
            result.x1 = x-w-LABEL_SP_X;
            result.y1 = y+LABEL_SP_Y-h/2;
            break;
        }

        case 6:
        {
            result.x1 = x-w-LABEL_SP_X;
            result.y1 = y+LABEL_SP_Y+h/2;
            break;
        }

        case 7:
        {
            result.x1 = x-w-LABEL_SP_X;
            result.y1 = y+LABEL_SP_Y;
            break;
        }

        default:
        {
            result.x1 = x+LABEL_SP_X;
            result.y1 = y+LABEL_SP_Y;
            break;
        }
    }
    bool bc = false;
    if(result.x1<20)
        result.x1 = 20;
    if((result.x1+w)>(EVE_HSIZE/2+R1+20))
    {
        result.x1 -= (result.x1+w) - (EVE_HSIZE/2+R1+20);
        result.y1 = y+LABEL_SP_Y+h/2;
        bc = true;
    }

    if(result.y1<h/2)
    {
        result.y1 = h/2;
    }
    else
        if((result.y1+h*2/3)>EVE_VSIZE)
        {
            result.y1 = EVE_VSIZE-(bc?h:h/2);
        }

    result.x2 = result.x1+w;
    result.y2 = result.y1+h;
    return result;
}


int32_t cATScreenEVE::getMaxLabelOverlap(sLabelsRect r)
{
    int32_t result = 0;
    for(auto itr=m_labels.begin(); itr!=m_labels.end(); itr++)
    {
        int32_t i = overlappingArea(r,*itr);
        if(i>result)
            result = i;
    }
    return result;
}

extern "C" { extern volatile uint16_t cmdOffset; }

uint16_t cATScreenEVE::getRadius()
{
    return m_global.getConfig().radius*m_distanceC;
}

void cATScreenEVE::drawPOI(sPOI &poi,int16_t count)
{
    uint16_t radius = m_global.getConfig().radius*m_distanceC;
    if(poi.isValidLocation() && poi.distance!=0)
    {
        double distance = poi.distance;
        bool drawAccuracy = distance<=radius;
        if(distance>radius)
        {
            if(poi.type==POIT_STATIC)
                return;
            distance = radius;
        }

        //if(poi.type==POIT_PEER)
        {
            int32_t cx = EVE_HSIZE/2-1;
            int32_t cy = EVE_VSIZE/2-1;

            int32_t x,y;
            int32_t tx,ty;
            sColor color(0,0,0);
            switch(poi.type)
            {
                case POIT_SELF:
                case POIT_PEER:
                {
                    switch(poi.status)
                    {
                        case POIS_NORMAL:
                        {
                            color = m_theme.peerNormalColor;
                            break;
                        }
                        case POIS_DEAD:
                        {
                            color = m_theme.peerDeadColor;
                            break;
                        }
                        default:
                        {
                            color = sColor(255,255,255);
                            break;
                        }
                    }
                    break;
                }
                case POIT_DYNAMIC:
                {
                    color = m_theme.peerDynamicColor;
                    break;
                }
                case POIT_STATIC:
                {
                    if(poi.color)
                        color = poi.color;
                    else
                        color = m_theme.peerStaticColor;
                    break;
                }
                case POIT_ENEMY:
                {
                    color = m_theme.peerEnemyColor;
                    break;
                }
            }
            EVE_cmd_dl(color.getEVEColor());
            double courseTo = m_heading+poi.courseTo*PI/180.0+PI/2;
            //DEBUG_MSG("%f %f %f\n",courseTo,m_heading,poi.courseTo*PI/180.0);
            calcCirclePoint(courseTo,R1*distance/radius,x,y);
            EVE_cmd_dl(BEGIN(EVE_POINTS));
            if(drawAccuracy && poi.hacc>0)
            {
                int32_t ps = R1*poi.hacc/(radius*10)*16;


                EVE_cmd_dl(STENCIL_OP(EVE_REPLACE,EVE_REPLACE));
                EVE_cmd_dl(STENCIL_MASK(4));
                EVE_cmd_dl(STENCIL_FUNC(EVE_ALWAYS,4,4));
                EVE_cmd_dl(COLOR_A(0));
                EVE_cmd_dl(POINT_SIZE(ps));
                EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));

                EVE_cmd_dl(STENCIL_OP(EVE_KEEP,EVE_KEEP));
                EVE_cmd_dl(STENCIL_MASK(255));
                
                EVE_cmd_dl(STENCIL_FUNC(EVE_EQUAL,1,1));
                EVE_cmd_dl((color+m_theme.peerAccuracyMask).getEVEColor());
                EVE_cmd_dl(COLOR_A(m_theme.peerAccuracyA));
                EVE_cmd_dl(POINT_SIZE(ps));
                EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));


                EVE_cmd_dl(STENCIL_FUNC(EVE_EQUAL,1,5));
                EVE_cmd_dl(COLOR_A(m_theme.peerAccuracyA*2));
                EVE_cmd_dl(POINT_SIZE(ps+32));
                EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));

                EVE_cmd_dl(COLOR_A(255));
                EVE_cmd_dl(STENCIL_FUNC(EVE_ALWAYS,0,255));
            }
            EVE_cmd_dl(POINT_SIZE(80));
            EVE_cmd_dl(VERTEX2II(x+cx,y+cy, 0, 0));
            EVE_cmd_dl(COLOR_RGB(0,0,0));
            char buffer[32];
            if(poi.symbol>0)
            {
                if(poi.symbol<30)
                    buffer[0] = poi.symbol+0x30;
                else
                    buffer[0] = poi.symbol;
            }
            else
            if(poi.name.c_str()[0])
                buffer[0] = poi.name.c_str()[0];
            else
                buffer[0] = ' ';
            buffer[1] = 0;
            if(poi.type!=POIT_SELF)
                EVE_cmd_text(x+cx+1,y+cy+1,EO_POI_FONT,EVE_OPT_CENTERX | EVE_OPT_CENTERY,buffer);
            
            if(count<=10)
            {
                tx = cx+x;
                ty = cy+y;

                int64_t delta = millis()-poi.lastUpdate;
                if(poi.type==POIT_SELF)
                {
                    snprintf(buffer,sizeof(buffer),"%dm",(int)poi.realDistance);
                }
                else
                if(poi.type==POIT_STATIC)
                {
                    snprintf(buffer,sizeof(buffer),"%dm",(int)poi.realDistance);
                }
                else
                {
                    if(delta>10000)
                        snprintf(buffer,sizeof(buffer),"%dm, %d%%, %ds",(int)poi.realDistance,(int)clamp((int)((poi.SNR + 10) * 5), 0, 100),(int)delta/1000);
                    else
                        snprintf(buffer,sizeof(buffer),"%dm, %d%%",(int)poi.realDistance,(int)clamp((int)((poi.SNR + 10) * 5), 0, 100));
                }
                int32_t w = max(strnlen(buffer,sizeof(buffer)),poi.name.size())*7;
                int32_t h = 30;
                sLabelsRect lr;
                sLabelsRect lrtmp = getLabelPos(tx,ty,w,h,0);
                lr = lrtmp;
                int32_t threshold = lrtmp.getArea()/10+5;
                int32_t minOverlap = getMaxLabelOverlap(lrtmp);
                if(poi.lastLabelIdx == 0)
                    minOverlap -= threshold;
                uint8_t activeLabelIdx = 0;
                uint8_t labelIdx = 1;
                while(minOverlap>0 && labelIdx<=LABEL_MAX_IDX)
                {
                    lrtmp = getLabelPos(tx,ty,w,h,labelIdx);
                    int32_t overlap = getMaxLabelOverlap(lrtmp);
                    if(labelIdx==poi.lastLabelIdx)
                        overlap -= threshold;
                    if(overlap<minOverlap)
                    {
                        activeLabelIdx = labelIdx;
                        lr = lrtmp;
                        minOverlap = overlap;
                    }
                    labelIdx++;
                }
                poi.lastLabelIdx = activeLabelIdx;
                m_labels.push_back(lr);
                tx = lr.x1;
                ty = lr.y1;
                //DEBUG_MSG("C 0x%0X\n",cmdOffset);

                EVE_cmd_dl(COLOR_RGB(0,0,0));
                EVE_cmd_text(tx+1,ty+14,EO_POI_FONT,EVE_OPT_CENTERY,buffer);
                EVE_cmd_dl(m_theme.peerTextColor.getEVEColor());
                EVE_cmd_text(tx,ty+13,EO_POI_FONT,EVE_OPT_CENTERY,buffer);

                snprintf(buffer,sizeof(buffer),"%s",poi.name.c_str());
                EVE_cmd_dl(COLOR_RGB(0,0,0));
                EVE_cmd_text(tx+1,ty+1,EO_POI_FONT,EVE_OPT_CENTERY,buffer);
                EVE_cmd_dl(m_theme.peerTextColor.getEVEColor());
                EVE_cmd_text(tx,ty,EO_POI_FONT,EVE_OPT_CENTERY,buffer);
            }
        }
    }
}

uint8_t eve_buffer[512];
bool EVE_loadImage(const char* filename, uint32_t ptr, uint32_t options)
{
    FILE *imgfile = fopen(filename,"r");
    if(imgfile)
    {
        size_t n = fread(eve_buffer,1,sizeof(eve_buffer),imgfile);
        if(n>0)
        {
            EVE_cmd_loadimage(ptr,options,eve_buffer,n);
        }
        while(n>0)
        {
            n = fread(eve_buffer,1,sizeof(eve_buffer),imgfile);
            if(n>0)
                block_transfer(eve_buffer,n);
        }
        fclose(imgfile);
        return true;
    }
    else
    {
        return false;
    }
    
}

bool EVE_inflateImage(const char* filename, uint32_t ptr)
{
    FILE *imgfile = fopen(filename,"r");
    if(imgfile)
    {
        size_t n = fread(eve_buffer,1,sizeof(eve_buffer),imgfile);
        if(n>0)
        {
            EVE_cmd_inflate(ptr,eve_buffer,n);
        }
        while(n>0)
        {
            n = fread(eve_buffer,1,sizeof(eve_buffer),imgfile);
            if(n>0)
                block_transfer(eve_buffer,n);
        }
        fclose(imgfile);
        return true;
    }
    else
    {
        return false;
    }
}

void cATScreenEVE::drawSideButtons()
{
    sSideButton *sb = m_global.m_sideButtons.getButtons();
    auto drawCaption = [=](uint16_t x,uint16_t y, uint8_t idx, uint16_t options) {
        EVE_cmd_dl(COLOR_RGB(0, 0, 0));
        EVE_cmd_text(x+1,y+1,EO_SIDE_BTN_FONT,options,sb[idx].caption.c_str());
        EVE_cmd_dl(m_theme.sideButtonsTextColor.getEVEColor());
        EVE_cmd_text(x,y,EO_SIDE_BTN_FONT,options,sb[idx].caption.c_str());
    };
    if(!sb[0].caption.empty())
        drawCaption(EO_SPACE,72,0,EVE_OPT_CENTERY);

    if(!sb[1].caption.empty())
        drawCaption(EO_SPACE,150,1,EVE_OPT_CENTERY);

    if(!sb[2].caption.empty())
        drawCaption(EO_SPACE,226,2,EVE_OPT_CENTERY);

    if(!sb[3].caption.empty())
        drawCaption(EVE_HSIZE-EO_SPACE,72,3,EVE_OPT_CENTERY|EVE_OPT_RIGHTX);

    if(!sb[4].caption.empty())
        drawCaption(EVE_HSIZE-EO_SPACE,150,4,EVE_OPT_CENTERY|EVE_OPT_RIGHTX);    

    if(!sb[5].caption.empty())
        drawCaption(EVE_HSIZE-EO_SPACE,226,5,EVE_OPT_CENTERY|EVE_OPT_RIGHTX);
}

uint32_t cATScreenEVE::flashProgrammStart()
{
    EVE_cmd_flashattach();
    return EVE_cmd_flashfast();
}

void cATScreenEVE::flashProgrammEnd()
{
}

#define RAMG_BUFFER (512*1024)
void cATScreenEVE::flashWrite(uint32_t address,uint16_t size, uint8_t *data)
{
    EVE_memWrite_buffer(RAMG_BUFFER,data,size,false);
    EVE_cmd_flashupdate(address,RAMG_BUFFER,size);
}

void cATScreenEVE::flashRead(uint32_t address,uint16_t size, uint8_t *data)
{
    EVE_cmd_flashread(RAMG_BUFFER,address,size);
    for(int i=0; i<size; i+=4)
       *((uint32_t *)&data[i]) = EVE_memRead32(i+RAMG_BUFFER);
}

void cATScreenEVE::showShutdownScreen()
{
    beginDraw();
	EVE_cmd_dl(DL_CLEAR_RGB | 0x000000UL);
	EVE_cmd_dl(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
    EVE_cmd_text(EVE_HSIZE/2,EVE_VSIZE-EO_BOOT_FONT*3-2,28,EVE_OPT_CENTERX,"Shutdown...");
    EVE_cmd_spinner(EVE_HSIZE/2,EVE_VSIZE/2,0,0);
    endDraw();
    EVE_cmd_execute();
}

void cATScreenEVE::showProgramMode()
{
    beginDraw();
	EVE_cmd_dl(DL_CLEAR_RGB | 0x000000UL);
	EVE_cmd_dl(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
    EVE_cmd_text(EVE_HSIZE/2,EVE_VSIZE-EO_BOOT_FONT*3-2,28,EVE_OPT_CENTERX,"Program mode...");
    EVE_cmd_spinner(EVE_HSIZE/2,EVE_VSIZE/2,0,0);
    endDraw();
    EVE_cmd_execute();
}

uint32_t cATScreenEVE::getFramesCount()
{
    return EVE_memRead32(REG_FRAMES);
}

void cATScreenEVE::showMenu()
{
    cATScreenLVGL::showMenu();
    beginDraw();
	EVE_cmd_dl(DL_CLEAR_RGB | 0x000000UL);
	EVE_cmd_dl(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
#ifdef FT81X_ARGB4
    EVE_cmd_setbitmap(0,EVE_ARGB4,CONFIG_LV_DISPLAY_WIDTH,CONFIG_LV_DISPLAY_HEIGHT);
#else
    EVE_cmd_setbitmap(0,EVE_RGB565,CONFIG_LV_DISPLAY_WIDTH,CONFIG_LV_DISPLAY_HEIGHT);
#endif    
    EVE_cmd_dl(BITMAP_SIZE(EVE_NEAREST,EVE_BORDER,EVE_BORDER,CONFIG_LV_DISPLAY_WIDTH,CONFIG_LV_DISPLAY_HEIGHT));

    EVE_cmd_dl(BEGIN(EVE_BITMAPS));
    EVE_cmd_dl(VERTEX2II(0, 0, 0, 0));
    EVE_cmd_dl(COLOR_RGB(m_theme.textColor.r,m_theme.textColor.g,m_theme.textColor.b));
    EVE_cmd_setfont2(15,EVE_HSIZE*EVE_VSIZE*2,32);
    EVE_cmd_dl(BITMAP_HANDLE(0));
    drawBattery();
    drawGPS();
    drawDateTime();
    drawSideButtons();
    endDraw();
    EVE_cmd_execute();
}

void cATScreenEVE::loop()
{
    if(m_screenState==SS_MENU)
    {
        cATScreenLVGL::loop();
    }
}

void cATScreenEVE::setScreenState(uint8_t value)
{
    cATScreenLVGL::setScreenState(value);
}

struct sEveTimerLine
{
    std::string str;
    bool finished;
};

void cATScreenEVE::drawTimers(bool forceAll)
{
    int64_t now = millis();
    std::list<sEveTimerLine> timersLines;
    std::stringstream ss;
    std::list<cTimer*> timers;
    int maxLineSize = 0;
    for(auto itr = m_global.m_timersChains.begin(); itr!=m_global.m_timersChains.end(); itr++)
    {
        auto timersList = (*itr)->getTimers();
        if((forceAll||(*itr)->isStarted())&&(*itr)->isEnabled())
        {
            int lineSize=0;
            for(auto titr=timersList.begin();titr!=timersList.end();titr++)
            {
                cTimer *t = (*titr);
                if(titr!=timersList.begin())
                {
                    ss<<" / ";
                    lineSize += 3;
                }
                lineSize += t->getName().length()+3;
                ss << t->getName() << " ";
                int duration = t->getDuration()-t->getCurrentDuration(now);
                ss << std::setfill('0') << std::setw(2) << duration/60000 << ":";
                ss << std::setfill('0') << std::setw(2) << duration%60000/1000;
            }
            if(lineSize>maxLineSize)
                maxLineSize = lineSize;
            sEveTimerLine line;
            line.str = ss.str();
            ss.str("");
            line.finished = (*itr)->isFinished(now);
            timersLines.push_back(line);
        }
        timers.insert(timers.end(),timersList.begin(),timersList.end());
    }
    timers.unique();
    for(auto itr=m_global.m_timers.begin();itr!=m_global.m_timers.end();itr++)
    {
        cTimer *t = itr->second;
        if(std::find(timers.begin(),timers.end(),t)==timers.end() && t->isEnabled())
        {
            if(forceAll||t->isStarted())
            {
                int lineSize = t->getName().length()+4;
                ss << t->getName() << " ";
                int duration = t->getDuration()-t->getCurrentDuration(now);
                ss << std::setfill('0') << std::setw(2) << duration/60000 << ":";
                ss << std::setfill('0') << std::setw(2) << duration%60000/1000;
                if(lineSize>maxLineSize)
                    maxLineSize = lineSize;
                sEveTimerLine line;
                line.str = ss.str();
                ss.str("");
                line.finished = t->isFinished(now);
                timersLines.push_back(line);
            }
        }
    }
    if(timersLines.size()>0)
    {
        int x,y;
        y = EVE_VSIZE-EO_SPACE-EO_TIMERS_FONT_SIZE*timersLines.size();
        x = EVE_HSIZE-EO_SPACE*2-maxLineSize*10;
        EVE_cmd_dl(m_theme.timerRectColor.getEVEColor());
        EVE_cmd_dl(COLOR_A(128));

        EVE_cmd_dl(BEGIN(EVE_RECTS));
        EVE_cmd_dl(VERTEX2II(x,y,0,0));
        EVE_cmd_dl(VERTEX2II(EVE_HSIZE,EVE_VSIZE,0,0));
        EVE_cmd_dl(END());
        EVE_cmd_dl(m_theme.timerTextColor.getEVEColor());
        EVE_cmd_dl(COLOR_A(255));
        x = x+EO_SPACE;
        y = y+EO_SPACE;
        for(auto itr = timersLines.begin(); itr!=timersLines.end(); itr++)
        {
            if(!(*itr).finished || (now%500)<=250)
            {
                EVE_cmd_text(x,y,EO_TIMERS_FONT,0,(*itr).str.c_str());
                if((*itr).finished)
                    EVE_cmd_text(x+1,y,EO_TIMERS_FONT,0,(*itr).str.c_str());
            }
            y += EO_TIMERS_FONT_SIZE;
        }
    }
}

/////////////////////////////////////////////////////////////
cMapsMemory::cMapsMemory(uint8_t count):m_count(count)
{
    m_memory = (sMapsMemoryItem*)calloc(m_count,sizeof(sMapsMemoryItem));
    for(uint8_t i=0; i<m_count; i++)
        m_memory[i].memoryaddress = i*MAX_MAP_IMAGE_SIZE;
}

cMapsMemory::~cMapsMemory()
{
    free(m_memory);
}

void cMapsMemory::clearCurrent()
{
    for(uint8_t i=0; i<m_count; i++)
        m_memory[i].inCurrentList = 0;
}

int8_t cMapsMemory::findItem(const sMapTile &tile)
{
    for(uint8_t i=0; i<m_count; i++)
        if(m_memory[i].inUse && m_memory[i].address==tile.address)
        {
            return i;
        }
    return -1;
}

void cMapsMemory::markCurrent(const sMapTile &tile)
{
    int8_t idx = findItem(tile);
    if(idx>=0)
        m_memory[idx].inCurrentList = 1;
}

bool cMapsMemory::allocItem(const sMapTile &tile,int64_t now, uint32_t &address, bool &needLoad)
{
    int8_t idx = findItem(tile);
    if(idx>=0)
    {
        m_memory[idx].lastUsed = now;
        needLoad = false;
        address = m_memory[idx].memoryaddress;
        return true;
    }
    for(uint8_t i=0; i<m_count; i++)
    {
        if(!m_memory[i].inUse)
        {
            idx = i;
            break;
        }
    }
    if(idx<0)
    {
        int64_t oldest = 0;
        for(uint8_t i=0; i<m_count; i++)
        {
            if(m_memory[i].inUse && !m_memory[i].inCurrentList)
            {
                if(idx<0 || m_memory[i].lastUsed<oldest)
                {
                    oldest = m_memory[i].lastUsed;
                    idx = i;
                }
            }
        }
    }
    if(idx>=0)
    {
        m_memory[idx].address = tile.address;
        m_memory[idx].inUse = 1;
        m_memory[idx].inCurrentList = 1;
        m_memory[idx].lastUsed = now;
        needLoad = true;
        address = m_memory[idx].memoryaddress;
        return true;
    }
    return false;
}
#endif