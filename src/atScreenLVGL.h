#pragma  once
#include "atscreen.h"
#include <lvgl_helpers.h>
#include "lvgl_mem.h"

#define LVS_NULL     0
#define LVS_MAINMENU 1
#define LVS_IMUCAL   2

#define LVS_MAX     2

class cATScreenLVGL;
class cLVGLScreen
{
friend cATScreenLVGL;
protected:
    cATScreenLVGL *m_owner;
    lv_obj_t *m_scr;
    lv_obj_t *m_mainCont;
    lv_obj_t *m_leftCont;
    lv_obj_t *m_rightCont;
    lv_style_t m_styleCont;

    bool m_initialized;
    virtual void createRoots();
    virtual void internalInit() = 0;
    virtual void internalDeinit() = 0;
    void setEventHandler(lv_obj_t *obj);
public:
    cLVGLScreen(cATScreenLVGL *owner);
    virtual ~cLVGLScreen();
    virtual void init();
    virtual void deinit();
    virtual void back()=0;
    virtual bool isMainPage()=0;
    virtual void loop(){};
    virtual void lvIScreenEvent(struct _lv_obj_t * obj, lv_event_t event) {}
};

class cATScreenLVGL:public cATScreen
{
friend cLVGLScreen;    
protected:
    uint8_t m_lvState;
    lv_disp_buf_t m_display_buffer;
    cLVGLScreen *m_screen;
    lv_group_t *m_keypadGroup;
    cLVGLScreen *m_screens[LVS_MAX];

    virtual cLVGLScreen * createScreen(uint8_t lvState);
 public:
    cATScreenLVGL(atGlobal &global);
    virtual ~cATScreenLVGL();

    virtual bool init();
    virtual void showMenu();
    virtual void loop();
    virtual void setScreenState(uint8_t value);
    virtual void processEvent(uint8_t event, uint32_t param1, uint32_t param2);
    virtual void setLVGLScreen(uint8_t value);
    lv_group_t *getKeypadGroup() { return m_keypadGroup; }
};
