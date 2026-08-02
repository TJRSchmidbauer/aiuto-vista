#include "ui.h"

void GameUiBuildStart(void);

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x07111C), 0);
    lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER, 0);
    ui_Image1 = NULL;
    GameUiBuildStart();
}
