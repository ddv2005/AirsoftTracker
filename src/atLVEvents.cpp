#include "atLVEvents.h"
#include "config.h"
#include "atscreen.h"

static uint32_t keycode_to_ascii(uint32_t key);

static uint32_t last_key = 0;
static lv_indev_state_t state ;
void atlve_init(void)
{
    /*Nothing to init*/
}

bool atlve_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    (void) indev_drv;      /*Unused*/
    data->state = state;
    data->key = keycode_to_ascii(last_key);
    lv_indev_t * indev = (lv_indev_t *)indev_drv->user_data;
    if(indev)
    {
        if(!lv_group_get_editing(indev->group))
        {
            switch(data->key)
            {
                case LV_KEY_UP:
                {
                    data->key = LV_KEY_LEFT;
                    break;
                }
                case LV_KEY_DOWN:
                {
                    data->key = LV_KEY_RIGHT;
                    break;
                }
            }
        }
    }
    return false;
}

/**
 * It is called periodically from the SDL thread to check a key is pressed/released
 * @param event describes the event
 */
void atlve_handler(uint8_t event, uint32_t param1, uint32_t param2)
{
    /* We only care about SDL_KEYDOWN and SDL_KEYUP events */
    switch(event) {
        case EV_BTN_ON:                       /*Button press*/
            last_key = param1;   /*Save the pressed key*/
            state = LV_INDEV_STATE_PR;          /*Save the key is pressed now*/
            break;
        case EV_BTN_OFF:                         /*Button release*/
            last_key = param1;   /*Save the pressed key*/
            state = LV_INDEV_STATE_REL;         /*Save the key is released but keep the last key*/
            break;
        default:
            break;

    }
}

static uint32_t keycode_to_ascii(uint32_t key)
{
    switch(key) {
        case 0:
            return LV_KEY_UP;

        case 1:
            return LV_KEY_DOWN;

        case 2:
            return LV_KEY_ESC;

        case 3:
            return LV_KEY_LEFT;

        case 4:
            return LV_KEY_ENTER;

        case 5:
            return LV_KEY_RIGHT;

        default:
            return 0;
    }
}
