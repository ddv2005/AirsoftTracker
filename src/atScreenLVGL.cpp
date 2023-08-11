#include "atScreenLVGL.h"
#include <lvgl.h>
#include "atLVEvents.h"
#include "lv_settings/lv_settings.h"
#include "global.h"

#define LV_SIDE_WIDTH 40
#define LV_TOP_HEIGHT 10

void *lvgl_malloc(size_t size)
{
    return heap_caps_malloc(size,MALLOC_CAP_SPIRAM);
}

void lvgl_free(void *ptr)
{
    heap_caps_free(ptr);
}

#pragma GCC diagnostic ignored "-Wwrite-strings"
static void lvIScreenEventCb(struct _lv_obj_t * obj, lv_event_t event)
{
  void *userData = lv_obj_get_user_data(obj);
  if(userData)
    ((cLVGLScreen*)userData)->lvIScreenEvent(obj,event);
}

void cLVGLScreen::setEventHandler(lv_obj_t *obj)
{
    lv_obj_set_user_data(obj,this);
    lv_obj_set_event_cb(obj,lvIScreenEventCb);
}

class cLVGLScreenMainMenu:public cLVGLScreen
{
protected:
    lv_settings_item_t m_rootItem = {.name = "Main Menu", .value = ""};

    lv_settings_item_t m_mainMenuItems[2] =
    {
        {.id = 10,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="Settings", .value=""},
        {.type = LV_SETTINGS_TYPE_INV},     /*Mark the last item*/
    };

    lv_settings_item_t m_mainSettingsItems[5] =
    {
        {.id = 4,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="Timers settings", .value="Timers durations"},
        {.id = 1,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="User settings", .value="ID, name, brightness, etc."},
        {.id = 2,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="Network settings", .value="Frequency, bandwidth etc."},
        {.id = 3,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="IMU settings", .value="Calibration, Settings"},
        {.type = LV_SETTINGS_TYPE_INV},     /*Mark the last item*/
    };

    char imAutoCal[16];
    lv_settings_item_t m_mainIMUItems[5] =
    {
        {.id = 1,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="Mag Calibration", .value=""},
        {.id = 2,.type = LV_SETTINGS_TYPE_SW, .name="Mag Auto Calibration", .value=imAutoCal},
        {.id = 3,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="Save Calibration", .value=""},
        {.id = 4,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="Reset Calibration", .value=""},
        {.type = LV_SETTINGS_TYPE_INV},     /*Mark the last item*/
    };

    char nmName[16];
    char nmUserId[4];
    char nmSymbol[2];
    lv_settings_item_t m_userMenuItems[4] =
    {
        {.id = 1,.type = LV_SETTINGS_TYPE_EDIT, .name="Name", .value=nmName, .param1=sizeof(nmName)},
        {.id = 2,.type = LV_SETTINGS_TYPE_EDIT, .name="Symbol", .value=nmSymbol, .param1=sizeof(nmSymbol)},
        {.id = 3,.type = LV_SETTINGS_TYPE_NUMSET, .name="ID", .value=nmUserId},
        {.type = LV_SETTINGS_TYPE_INV},     /*Mark the last item*/
    };

    char nmFrequency[16];
    char nmNetworkId[4];
    char nmSF[4];
    char nmCR[4];
    char nmPreamble[4];
    char nmSync[6];
    char nmPassword[16];
    lv_settings_item_t m_networkMenuItems[10] =
    {
        {.id = 1,.type = LV_SETTINGS_TYPE_NUMSET, .name="Frequency", .value=nmFrequency},
        {.id = 2,.type = LV_SETTINGS_TYPE_DDLIST, .name="Bandwidth", .value="500Mhz\n250Mhz"},
        {.id = 3,.type = LV_SETTINGS_TYPE_NUMSET, .name="Spreading Factor", .value=nmSF},
        {.id = 4,.type = LV_SETTINGS_TYPE_NUMSET, .name="Coding Rate", .value=nmCR},
        {.id = 5,.type = LV_SETTINGS_TYPE_NUMSET, .name="Preamble", .value=nmPreamble},
        {.id = 6,.type = LV_SETTINGS_TYPE_NUMSET, .name="Sync Word", .value=nmSync},
        {.id = 10,.type = LV_SETTINGS_TYPE_NUMSET, .name="Network ID", .value=nmNetworkId},
        {.id = 11,.type = LV_SETTINGS_TYPE_EDIT, .name="Password", .value=nmPassword, .param1=sizeof(nmPassword),.param2=LV_SETTINGS_EDIT_PWD},
        {.id = 20,.type = LV_SETTINGS_TYPE_LIST_BTN, .name="Save", .value="Save network settings"},        
        {.type = LV_SETTINGS_TYPE_INV},     /*Mark the last item*/
    };

    char nmTimers[AT_MAX_TIMERS][10];
    lv_settings_item_t m_timersSettings[5] =
    {
        {.id = 0,.type = LV_SETTINGS_TYPE_NUMSET, .name="Timer 1", .value=nmTimers[0], .param1 = 10},
        {.id = 1,.type = LV_SETTINGS_TYPE_NUMSET, .name="Timer 2", .value=nmTimers[1], .param1 = 10},
        {.id = 2,.type = LV_SETTINGS_TYPE_NUMSET, .name="Timer 3", .value=nmTimers[2], .param1 = 10},
        {.id = 3,.type = LV_SETTINGS_TYPE_NUMSET, .name="Timer 4", .value=nmTimers[3], .param1 = 10},
        {.type = LV_SETTINGS_TYPE_INV},     /*Mark the last item*/
    };

    lv_obj_t *m_root;
    bool m_isMainPage = true;
    virtual void internalInit();
    virtual void internalDeinit();
    void updateIMUSettings();
    void updateNetworkSettings();
    void updateTimersSettings();
    void updateUserSettings();
public:
    cLVGLScreenMainMenu(cATScreenLVGL *owner):cLVGLScreen(owner){};
    void mainMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e);
    void settingsMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e);
    void networkMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e);
    void userMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e);
    void timersMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e);
    void imuMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e);
    virtual void back()
    {
        lv_settings_back();
    }
    virtual bool isMainPage() { return m_isMainPage; }
};

struct lv_mcal_axis_s
{
    lv_obj_t* page;
    lv_obj_t* label;
    lv_obj_t* btnP;
    lv_obj_t* btnM;
};
class cLVGLScreenIMUCalibration:public cLVGLScreen
{
protected:
    int64_t m_updateTime;
    lv_obj_t *m_root;
    lv_style_t text48Style;
    lv_style_t text16Style;
    lv_style_t text14Style;
    lv_mcal_axis_s m_axisX;  
    lv_mcal_axis_s m_axisY;
    lv_mcal_axis_s m_axisZ;
    lv_obj_t* m_headingLabel;
    lv_obj_t* m_autoButton;
    virtual void internalInit();
    virtual void internalDeinit();
    lv_obj_t* lv_button_create(lv_obj_t* parent, const char* caption);
    lv_mcal_axis_s lv_mcal_axis_create(lv_obj_t* parent);
    void updateIMU();
public:
    cLVGLScreenIMUCalibration(cATScreenLVGL *owner):cLVGLScreen(owner){};
    virtual void back()
    {
        m_owner->setLVGLScreen(LVS_MAINMENU);
    }
    virtual void loop();
    virtual bool isMainPage() { return false; }
    virtual void lvIScreenEvent(struct _lv_obj_t * obj, lv_event_t event);
};

cATScreenLVGL::cATScreenLVGL(atGlobal &global):cATScreen(global)
{
    m_lvState = LVS_NULL;
    m_screen = NULL;
    m_keypadGroup = NULL;
    memset(m_screens,0,sizeof(m_screens));
}

cATScreenLVGL::~cATScreenLVGL()
{

}

bool cATScreenLVGL::init()
{
    if(cATScreen::init())
    {
        lv_init();
        lvgl_driver_init();

        static lv_color_t  m_buffer1[DISP_BUF_SIZE];
        uint32_t size_in_px = DISP_BUF_SIZE;
        lv_disp_buf_init(&m_display_buffer, m_buffer1, NULL, size_in_px);

        lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);
        disp_drv.flush_cb = disp_driver_flush;

        disp_drv.buffer = &m_display_buffer;
        lv_disp_drv_register(&disp_drv);

        m_keypadGroup = lv_group_create();

        lv_indev_drv_t kb_drv;
        lv_indev_drv_init(&kb_drv);
        kb_drv.type = LV_INDEV_TYPE_ENCODER;
        kb_drv.read_cb = atlve_read;
        lv_indev_t * kb_indev = lv_indev_drv_register(&kb_drv);
        kb_indev->driver.user_data = kb_indev;
        lv_indev_set_group(kb_indev, m_keypadGroup);

        return true;
    }
    else
    {
        return false;
    }    
}

void cATScreenLVGL::showMenu()
{
     
}

void cATScreenLVGL::loop()
{
    lv_task_handler();
    if(m_screen)
        m_screen->loop();
}

void cATScreenLVGL::setLVGLScreen(uint8_t value)
{
    if(value!=m_lvState)
    {
        m_lvState = value;
        if(m_screen)
        {
            m_screen->deinit();
            m_screen = NULL;
        }
    }
    if(m_screen==NULL)
    {
        m_screen = createScreen(m_lvState);
        if(m_screen)
            m_screen->init();
    }
}

void cATScreenLVGL::setScreenState(uint8_t value)
{
    if(value!=m_screenState)
    {
        cATScreen::setScreenState(value);
        switch(m_screenState)
        {
            case SS_MENU:
            {
                setLVGLScreen(LVS_MAINMENU);
                break;
            }
        }
        m_global.onScreenStateChange();
    }
}

cLVGLScreen * cATScreenLVGL::createScreen(uint8_t lvState)
{
    cLVGLScreen *result = m_screens[lvState-1];
    if(result)
        return result;
    switch(lvState)
    {
        case LVS_MAINMENU:
        {
            result =  new cLVGLScreenMainMenu(this);
            break;
        }

        case LVS_IMUCAL:
        {
            result = new cLVGLScreenIMUCalibration(this);
            break;
        }
    }
    m_screens[lvState-1] = result;
    return result;
}

void cATScreenLVGL::processEvent(uint8_t event, uint32_t param1, uint32_t param2)
{
    switch(event)
    {
        case EV_BTN_ON:
        {
            if(param1==2)
            {
                if(m_lvState > LVS_NULL)
                {
                    if(m_screen)
                    {
                        if(m_screen->isMainPage())
                            setScreenState(SS_MAIN);
                        else
                            m_screen->back();
                    }
                    else
                        setScreenState(SS_MAIN);
                }
            }
            break;
        }

    };
    atlve_handler(event,param1,param2);
}

cLVGLScreen::cLVGLScreen(cATScreenLVGL *owner):m_owner(owner)
{
    m_initialized = false;
    m_scr = NULL;
    m_mainCont = NULL;
    m_leftCont = NULL;
    m_rightCont = NULL;
}

cLVGLScreen::~cLVGLScreen()
{

}

void cLVGLScreen::createRoots()
{
    lv_style_init(&m_styleCont);
    lv_style_set_radius(&m_styleCont,LV_STATE_DEFAULT,0);
    lv_style_set_border_width(&m_styleCont, LV_STATE_DEFAULT, 0);

    m_scr = lv_obj_create(NULL, NULL);
    lv_scr_load(m_scr);
    m_leftCont = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_add_style(m_leftCont,LV_CONT_PART_MAIN, &m_styleCont);
    lv_obj_set_size(m_leftCont, LV_SIDE_WIDTH, lv_obj_get_height(lv_scr_act()));
    lv_obj_set_pos(m_leftCont,0,0);

    m_mainCont = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_add_style(m_mainCont,LV_CONT_PART_MAIN, &m_styleCont);
    lv_obj_set_size(m_mainCont, lv_obj_get_width(lv_scr_act())-LV_SIDE_WIDTH*2, lv_obj_get_height(lv_scr_act())-LV_TOP_HEIGHT);
    lv_obj_set_pos(m_mainCont,LV_SIDE_WIDTH,LV_TOP_HEIGHT);

    m_rightCont = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_add_style(m_rightCont,LV_CONT_PART_MAIN, &m_styleCont);
    lv_obj_set_size(m_rightCont, LV_SIDE_WIDTH, lv_obj_get_height(lv_scr_act()));
    lv_obj_set_pos(m_rightCont,lv_obj_get_width(lv_scr_act())-LV_SIDE_WIDTH,0);
}

void cLVGLScreen::init()
{
    if(!m_initialized)
    {
        internalInit();
        m_initialized = true;
    }
}

void cLVGLScreen::deinit()
{
    if(m_initialized)
    {
        internalDeinit();
        m_initialized = false;
    }
}

////////////////////////////////////////////
static cLVGLScreenMainMenu *menuObject=NULL;

#define DEF_SETTINGS_CB(name) \
static void  name##Cb(lv_obj_t * obj, lv_event_t e) \
{\
    lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();\
    if(menuObject)\
        menuObject->name(act_item,obj,e);\
}\

DEF_SETTINGS_CB(mainMenuEvent)
DEF_SETTINGS_CB(settingsMenuEvent)
DEF_SETTINGS_CB(networkMenuEvent)
DEF_SETTINGS_CB(userMenuEvent)
DEF_SETTINGS_CB(timersMenuEvent)
DEF_SETTINGS_CB(imuMenuEvent)


void cLVGLScreenMainMenu::internalInit()
{
    if(m_scr)
    {
        lv_scr_load(m_scr);
        if(!lv_settings_reopen_last())
            lv_settings_open_page(&m_rootItem, mainMenuEventCb);
    }
    else
    {
        menuObject = this;

        lv_settings_set_group(m_owner->getKeypadGroup());
        
        createRoots();
        m_root = lv_settings_create(&m_rootItem, NULL);
        lv_obj_set_hidden(m_root,true);
        lv_settings_set_parent(m_mainCont);
        lv_settings_set_max_width(lv_obj_get_width(lv_scr_act())-LV_SIDE_WIDTH*2);
        lv_settings_open_page(&m_rootItem, mainMenuEventCb);
    }
}

void cLVGLScreenMainMenu::internalDeinit()
{
}

void cLVGLScreenMainMenu::mainMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_REFRESH) 
    {
        m_isMainPage = true;
        for(uint32_t i = 0; m_mainMenuItems[i].type != LV_SETTINGS_TYPE_INV; i++) {
            lv_settings_add(&m_mainMenuItems[i]);
        }    
    }
    else if(e == LV_EVENT_CLICKED) {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        switch(act_item->id)
        {
            case 10:  //Settings
            {
                lv_settings_open_page(act_item, settingsMenuEventCb);
                break;
            }
        }
    }    

}

void cLVGLScreenMainMenu::settingsMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_REFRESH) 
    {
        global->updateTimers();
        m_isMainPage = false;
        for(uint32_t i = 0; m_mainSettingsItems[i].type != LV_SETTINGS_TYPE_INV; i++) {
            lv_settings_add(&m_mainSettingsItems[i]);
        }    
    }
    else if(e == LV_EVENT_CLICKED) {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        switch(act_item->id)
        {
            case 1:  //User settings
            {
                lv_settings_open_page(act_item, userMenuEventCb);
                break;
            }
            case 2:  //Network settings
            {
                lv_settings_open_page(act_item, networkMenuEventCb);
                break;
            }
            case 3:  //IMU settings
            {
                lv_settings_open_page(act_item, imuMenuEventCb);
                break;
            }

            case 4:  //Timers settings
            {
                lv_settings_open_page(act_item, timersMenuEventCb);
                break;
            }

        }
    }    
}

void cLVGLScreenMainMenu::updateIMUSettings()
{
    snprintf(imAutoCal,sizeof(imAutoCal),"%s",global->getIMU()->getAutoMag()?"Enabled":"Disabled");
    m_mainIMUItems[1].state = global->getIMU()->getAutoMag()?1:0;
}

void cLVGLScreenMainMenu::imuMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_REFRESH) 
    {
        updateIMUSettings();
        m_isMainPage = false;
        for(uint32_t i = 0; m_mainIMUItems[i].type != LV_SETTINGS_TYPE_INV; i++) {
            lv_settings_add(&m_mainIMUItems[i]);
        }    
    }
    else if(e == LV_EVENT_CLICKED) {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        switch(act_item->id)
        {
            case 1:  //IMU Calibration
            {
                m_owner->setLVGLScreen(LVS_IMUCAL);
                break;
            }
            case 3:  //Save
            {
                global->getIMU()->saveCalibration();
                break;
            }

            case 4:  //Reset
            {
                global->getIMU()->loadCalibration();
                break;
            }

        }
    }
    else if(e == LV_EVENT_VALUE_CHANGED)    
    {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        switch(act_item->id)
        {
            case 2:  //Auto
            {
                global->getIMU()->setAutoMag(act_item->state);
                updateIMUSettings();
                lv_settings_refr(act_item);
                break;
            }
        }

    }
}

void cLVGLScreenMainMenu::networkMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_REFRESH) 
    {
        m_isMainPage = false;
        updateNetworkSettings();
        for(uint32_t i = 0; m_networkMenuItems[i].type != LV_SETTINGS_TYPE_INV; i++) {
            lv_settings_add(&m_networkMenuItems[i]);
        }    
    }
    else if(e == LV_EVENT_CLICKED) {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        switch(act_item->id)
        {
            case 20:  //Save
            {
                global->getNetworkConfig().saveConfig(NET_CONFIG_FILENAME);
                break;
            }
        }
    }
    else if(e == LV_EVENT_VALUE_CHANGED) {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        switch(act_item->id)
        {
            case 1:  //Frequency
            {
                if(act_item->state>9300)
                    act_item->state=9300;
                else
                if(act_item->state<8800)
                    act_item->state=8800;
                global->getNetworkConfig().channelConfig.freq = act_item->state/10.0;
                updateNetworkSettings();
                break;
            }
            case 2:  //BW
            {
                switch(act_item->state)
                {
                    case 0:
                    {
                        global->getNetworkConfig().channelConfig.bw = 500.0;
                        break;
                    }
                    case 1:
                    {
                        global->getNetworkConfig().channelConfig.bw = 250.0;
                        break;
                    }
                }
                updateNetworkSettings();
                break;
            }
            case 3:  //SF
            {
                if(act_item->state>10)
                    act_item->state=10;
                else
                if(act_item->state<7)
                    act_item->state=7;
                global->getNetworkConfig().channelConfig.sf = act_item->state;
                updateNetworkSettings();
                break;
            }

            case 4:  //CR
            {
                if(act_item->state>8)
                    act_item->state=8;
                else
                if(act_item->state<5)
                    act_item->state=5;
                global->getNetworkConfig().channelConfig.cr = act_item->state;
                updateNetworkSettings();
                break;
            }

            case 5:  //Preamble
            {
                if(act_item->state>16)
                    act_item->state=16;
                else
                if(act_item->state<4)
                    act_item->state=4;
                global->getNetworkConfig().channelConfig.preambleLength = act_item->state;
                updateNetworkSettings();
                break;
            }

            case 6:  //Sync
            {
                if(act_item->state>255)
                    act_item->state=255;
                else
                if(act_item->state<1)
                    act_item->state=1;
                global->getNetworkConfig().channelConfig.syncWord = act_item->state;
                updateNetworkSettings();
                break;
            }

            case 10: //ID
            {
                if(act_item->state<0)
                    act_item->state=0;
                if(act_item->state>=128)
                    act_item->state=127;
                global->getNetworkConfig().networkId = act_item->state;
                updateNetworkSettings();
                break;
            }

            case 11:  //Password
            {
                global->getNetworkConfig().password = act_item->value;
                updateNetworkSettings();
                break;
            }
        }
    }     
}

void cLVGLScreenMainMenu::userMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_REFRESH) 
    {
        m_isMainPage = false;
        updateUserSettings();
        for(uint32_t i = 0; m_userMenuItems[i].type != LV_SETTINGS_TYPE_INV; i++) {
            lv_settings_add(&m_userMenuItems[i]);
        }    
    }
    else if(e == LV_EVENT_VALUE_CHANGED) {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        switch(act_item->id)
        {
            case 1:  //Name
            {
                global->getConfig().name = act_item->value;
                updateUserSettings();
                break;
            }
            case 2:  //Symbol
            {
                int len = strnlen(act_item->value,act_item->param1);
                if(len>0)
                {
                    if(global->getConfig().symbol!=act_item->value[len-1])
                    {
                        global->getConfig().symbol = act_item->value[len-1];
                    }
                    else
                        if(len==1)
                            break;
                }
                updateUserSettings();
                lv_settings_refr(act_item);
                break;
            }
            case 3: //ID
            {
                if(act_item->state<1)
                    act_item->state=1;
                if(act_item->state>=128)
                    act_item->state=127;
                global->getConfig().id = act_item->state;
                updateUserSettings();
                break;
            }
        }
    }     
}

#define LVGL_HOFFSET 40
#define LVGL_VOFFSET 14
#define LVGL_HPAD 10
#define LVGL_VPAD 6

void cLVGLScreenIMUCalibration::internalInit()
{
    int16_t width = LV_HOR_RES_MAX - LVGL_HOFFSET * 2;
    int16_t height = LV_VER_RES_MAX - LVGL_VOFFSET * 2;

    m_updateTime = millis();
    createRoots();
    lv_group_remove_all_objs(m_owner->getKeypadGroup());
    lv_obj_t* mainPage = lv_cont_create(lv_scr_act(), NULL);
    m_root = mainPage;
    lv_obj_set_pos(mainPage, LVGL_HOFFSET, LVGL_VOFFSET);
    lv_obj_set_size(mainPage, width,height);

    width = lv_obj_get_width(mainPage)-LVGL_HPAD * 2;
    height = lv_obj_get_height(mainPage) - LVGL_VPAD * 2;

    lv_style_init(&text48Style);
    lv_style_set_text_font(&text48Style, LV_STATE_DEFAULT, &lv_font_montserrat_48);

    lv_style_init(&text16Style);
    lv_style_set_text_font(&text16Style, LV_STATE_DEFAULT, &lv_font_montserrat_16);

    lv_style_init(&text14Style);
    lv_style_set_text_font(&text14Style, LV_STATE_DEFAULT, &lv_font_montserrat_14);

    lv_obj_t* captionLabel = lv_label_create(mainPage, NULL);
    lv_label_set_text(captionLabel, "Magnetometer Calibration");
    lv_label_set_align(captionLabel, LV_LABEL_ALIGN_CENTER);
    lv_obj_add_style(captionLabel, 0, &text16Style);
    lv_obj_align(captionLabel, mainPage, LV_ALIGN_IN_TOP_MID,0, LVGL_VPAD);

    m_axisX = lv_mcal_axis_create(mainPage);
    lv_obj_set_pos(m_axisX.page, 4, LVGL_VPAD + 20);

    m_axisY = lv_mcal_axis_create(mainPage);
    lv_obj_set_pos(m_axisY.page, 80, LVGL_VPAD + 20);

    m_axisZ = lv_mcal_axis_create(mainPage);
    lv_obj_set_pos(m_axisZ.page, 156, LVGL_VPAD + 20);

//    lv_obj_t* s90Button = lv_button_create(mainPage, "90°");
//    lv_obj_set_pos(s90Button, 20, 100);
//    lv_obj_set_size(s90Button, 60, 26);

    m_autoButton = lv_button_create(mainPage, "Auto");
    lv_btn_set_checkable(m_autoButton, true);
    lv_obj_set_pos(m_autoButton, 90, 100);
    lv_obj_set_size(m_autoButton, 60, 26);
    setEventHandler(m_autoButton);

//    lv_obj_t* s0Button = lv_button_create(mainPage, "0°");
//    lv_obj_set_pos(s0Button, 160, 100);
//    lv_obj_set_size(s0Button, 60, 26);

    lv_obj_t* o = lv_cont_create(mainPage, NULL);
    lv_obj_set_pos(o, LVGL_HPAD, 130);
    lv_obj_set_size(o, width, 76);

    m_headingLabel = lv_label_create(o, NULL);
    lv_label_set_text(m_headingLabel, "000°");
    lv_obj_add_style(m_headingLabel, 0, &text48Style);
    lv_obj_align(m_headingLabel, o, LV_ALIGN_CENTER, 0, 0);
    updateIMU();
}

void cLVGLScreenIMUCalibration::internalDeinit()
{
    lv_obj_del(m_scr);
    m_scr = NULL;
}

lv_obj_t* cLVGLScreenIMUCalibration::lv_button_create(lv_obj_t* parent, const char* caption)
{
    lv_obj_t* result = lv_btn_create(parent, NULL);
    lv_btn_set_fit(result, LV_FIT_NONE);
    lv_obj_t* label = lv_label_create(result, NULL);
    lv_label_set_text(label, caption);
    lv_obj_add_style(label, 0, &text14Style);
    lv_obj_align(label, result, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(m_owner->getKeypadGroup(),result);

    return result;
}
lv_mcal_axis_s cLVGLScreenIMUCalibration::lv_mcal_axis_create(lv_obj_t* parent)
{
    lv_obj_t* axisPage = lv_cont_create(parent, NULL);
    lv_obj_set_size(axisPage, 80, 65);

    lv_obj_t* axislabel = lv_label_create(axisPage, NULL);
    lv_label_set_text(axislabel, "-00.00");
    lv_obj_align(axislabel, axisPage, LV_ALIGN_IN_TOP_MID, 0, 5);

    lv_obj_t* axisPBtn = lv_btn_create(axisPage, NULL);
    lv_obj_set_pos(axisPBtn, 6, 30);
    lv_obj_set_size(axisPBtn, 30, 26);
    lv_btn_set_fit(axisPBtn, LV_FIT_NONE);
    setEventHandler(axisPBtn);
    lv_obj_t* label = lv_label_create(axisPBtn, NULL);
    lv_label_set_text(label, "+");
    lv_obj_add_style(label, 0, &text14Style);
    lv_obj_align(label, axisPBtn, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(m_owner->getKeypadGroup(),axisPBtn);

    lv_obj_t* axisMBtn = lv_btn_create(axisPage, NULL);
    lv_obj_set_pos(axisMBtn, 42, 30);
    lv_obj_set_size(axisMBtn, 30, 26);
    lv_btn_set_fit(axisMBtn, LV_FIT_NONE);
    lv_obj_set_user_data(axisMBtn,this);
    setEventHandler(axisMBtn);
    label = lv_label_create(axisMBtn, NULL);
    lv_label_set_text(label, "-");
    lv_obj_add_style(label, 0, &text14Style);
    lv_obj_align(label, axisMBtn, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(m_owner->getKeypadGroup(),axisMBtn);

    lv_mcal_axis_s result;
    result.page = axisPage;
    result.label = axislabel;
    result.btnP = axisPBtn;
    result.btnM = axisMBtn;
    return result;
}


void cLVGLScreenIMUCalibration::updateIMU()
{
    if(global->m_imuStatus)
    {
        char buffer[32];
        snprintf(buffer,sizeof(buffer),"%d°",(int)(global->m_imuStatus->getHeading()/100+global->m_gpsStatus->getMagDec()));
        lv_label_set_text(m_headingLabel,buffer);
        lv_obj_align(m_headingLabel, lv_obj_get_parent(m_headingLabel), LV_ALIGN_CENTER, 0, 0);

        global->getIMU()->updateCalibration();
        sIMUCalibration &calibration = global->getIMU()->getCalibration();
        snprintf(buffer,sizeof(buffer),"X: %0.2f",calibration.magBiasX);
        lv_label_set_text(m_axisX.label,buffer);
        lv_obj_align(m_axisX.label, lv_obj_get_parent(m_axisX.label), LV_ALIGN_IN_TOP_MID, 0, 5);

        snprintf(buffer,sizeof(buffer),"Y: %0.2f",calibration.magBiasY);
        lv_label_set_text(m_axisY.label,buffer);
        lv_obj_align(m_axisY.label, lv_obj_get_parent(m_axisY.label), LV_ALIGN_IN_TOP_MID, 0, 5);

        snprintf(buffer,sizeof(buffer),"Z: %0.2f",calibration.magBiasZ);
        lv_label_set_text(m_axisZ.label,buffer);
        lv_obj_align(m_axisZ.label, lv_obj_get_parent(m_axisZ.label), LV_ALIGN_IN_TOP_MID, 0, 5);

        if(global->getIMU()->getAutoMag())
            lv_obj_add_state(m_autoButton, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(m_autoButton, LV_STATE_CHECKED);
    }
}

void cLVGLScreenIMUCalibration::loop()
{
    if((millis()-m_updateTime)>=150)
    {
        updateIMU();
        m_updateTime = millis();
    }
}

#define MAG_ADJ_VAL 0.1

void cLVGLScreenIMUCalibration::lvIScreenEvent(struct _lv_obj_t * obj, lv_event_t event)
{
    switch(event)
    {
        case LV_EVENT_CLICKED:
        {
            if(obj==m_autoButton)
            {
                global->getIMU()->setAutoMag(lv_obj_get_state(m_autoButton,LV_OBJ_PART_MAIN) & LV_STATE_CHECKED);
            }
            else
            if(obj==m_axisX.btnM)
            {
                global->getIMU()->adjustMagCalibrationBias(-MAG_ADJ_VAL,0);
            }
            else
            if(obj==m_axisX.btnP)
            {
                global->getIMU()->adjustMagCalibrationBias(MAG_ADJ_VAL,0);
            }
            else
            if(obj==m_axisY.btnM)
            {
                global->getIMU()->adjustMagCalibrationBias(-MAG_ADJ_VAL,1);
            }
            else
            if(obj==m_axisY.btnP)
            {
                global->getIMU()->adjustMagCalibrationBias(MAG_ADJ_VAL,1);
            }
            else
            if(obj==m_axisZ.btnM)
            {
                global->getIMU()->adjustMagCalibrationBias(-MAG_ADJ_VAL,2);
            }
            else
            if(obj==m_axisZ.btnP)
            {
                global->getIMU()->adjustMagCalibrationBias(MAG_ADJ_VAL,2);
            }
            updateIMU();
            break;
        }
    }
}

void cLVGLScreenMainMenu::updateTimersSettings()
{
    atMainConfig &c = global->getConfig();
    for(int i=0; i<AT_MAX_TIMERS; i++)
    {
        m_timersSettings[i].state = c.timers[i]/1000;
        snprintf(nmTimers[i],sizeof(nmTimers[i]),"%d",c.timers[i]/1000);
    }
}

void cLVGLScreenMainMenu::updateNetworkSettings()
{
    atNetworkConfig &nc = global->getNetworkConfig();
    m_networkMenuItems[0].state = nc.channelConfig.freq*10;
    snprintf(nmFrequency,sizeof(nmFrequency),"%03.1f Mhz",m_networkMenuItems[0].state/10.0);
    m_networkMenuItems[1].state = 0;
    if(nc.channelConfig.bw==250)
        m_networkMenuItems[1].state = 1;

    m_networkMenuItems[2].state = nc.channelConfig.sf;
    snprintf(nmSF,sizeof(nmSF),"%d",nc.channelConfig.sf);

    m_networkMenuItems[3].state = nc.channelConfig.cr;
    snprintf(nmCR,sizeof(nmCR),"%d",nc.channelConfig.cr);

    m_networkMenuItems[4].state = nc.channelConfig.preambleLength;
    snprintf(nmPreamble,sizeof(nmPreamble),"%d",nc.channelConfig.preambleLength);

    m_networkMenuItems[5].state = nc.channelConfig.syncWord;
    snprintf(nmSync,sizeof(nmSync),"0x%0X",nc.channelConfig.syncWord);

    m_networkMenuItems[6].state = nc.networkId;
    snprintf(nmNetworkId,sizeof(nmNetworkId),"%d",nc.networkId);

    snprintf(nmPassword,sizeof(nmPassword),"%s",nc.password.c_str());
}


void cLVGLScreenMainMenu::updateUserSettings()
{
    atMainConfig &c = global->getConfig();
    snprintf(nmName,sizeof(nmName),"%s",c.name.c_str());
    snprintf(nmSymbol,sizeof(nmSymbol),"%c",c.symbol);
    m_userMenuItems[1].state = c.id;
    snprintf(nmUserId,sizeof(nmUserId),"%d",c.id);
}

void cLVGLScreenMainMenu::timersMenuEvent(lv_settings_item_t * act_item, lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_REFRESH) 
    {
        m_isMainPage = false;
        updateTimersSettings();
        for(uint32_t i = 0; m_timersSettings[i].type != LV_SETTINGS_TYPE_INV; i++) {
            lv_settings_add(&m_timersSettings[i]);
        }    
    }
    else if(e == LV_EVENT_VALUE_CHANGED) {
        lv_settings_item_t * act_item = (lv_settings_item_t *)lv_event_get_data();
        if(act_item->id<AT_MAX_TIMERS)
        {
            if(act_item->state<0)
                act_item->state = 0;
            global->getConfig().timers[act_item->id] = act_item->state*1000;
            updateTimersSettings();
        }
    }     
}
