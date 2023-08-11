#include "app.h"

/**********************
 *       WIDGETS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/


void app_create(lv_obj_t *parent)
{

	static lv_style_t toplabel_s0;
	lv_style_init(&toplabel_s0);
	lv_style_set_text_font(&toplabel_s0,LV_STATE_DEFAULT,&Roboto 16);
	lv_style_set_text_font(&toplabel_s0,LV_STATE_CHECKED,&Roboto 16);
	lv_style_set_text_font(&toplabel_s0,LV_STATE_FOCUSED,&Roboto 16);
	lv_style_set_text_font(&toplabel_s0,LV_STATE_EDITED,&Roboto 16);
	lv_style_set_text_font(&toplabel_s0,LV_STATE_HOVERED,&Roboto 16);
	lv_style_set_text_font(&toplabel_s0,LV_STATE_PRESSED,&Roboto 16);
	lv_style_set_text_font(&toplabel_s0,LV_STATE_DISABLED,&Roboto 16);
	lv_obj_t *toplabel = lv_label_create(parent, NULL);
	lv_label_set(toplabel,"Main Menu")
	lv_label_set_align(toplabel, LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(toplabel, LV_LABEL_LONG_DOT);
	lv_obj_set_pos(toplabel, 0, 2);
	lv_obj_set_size(toplabel, 320, 19);
	lv_obj_add_style(toplabel, 0, &toplabel_s0);

}
