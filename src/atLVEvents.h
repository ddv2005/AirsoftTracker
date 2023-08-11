#ifndef ATLVE_H
#define ATLVE_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

void atlve_init(void);
bool atlve_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
void atlve_handler(uint8_t event, uint32_t param1, uint32_t param2);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
