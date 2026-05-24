#include "ui_dashboard.h"

static lv_obj_t * ui_Screen1;
static lv_obj_t * ui_mater_zhuansu;
static lv_obj_t * ui_mater_speed;
static lv_meter_indicator_t * indic_zuansu;
static lv_meter_indicator_t * indic_speed;

static lv_obj_t * ui_label_rpm;
static lv_obj_t * ui_label_speed;
static lv_obj_t * ui_Bar1;
static lv_obj_t * ui_bar_elc;
static lv_obj_t * ui_lbl_tmp;
static lv_obj_t * ui_lbl_elc;

static lv_obj_t * ui_aqd;
static lv_obj_t * ui_warn;
static lv_obj_t * ui_sc;
static lv_obj_t * ui_left;
static lv_obj_t * ui_right;

static uint8_t visible = 0;

// 声明外部图标资源
LV_IMG_DECLARE(ui_img_left_png);      // 左转向灯
LV_IMG_DECLARE(ui_img_right_png);     // 右转向灯
LV_IMG_DECLARE(ui_img_sc_png);        // 手刹
LV_IMG_DECLARE(ui_img_qeq_png);       // 安全带
LV_IMG_DECLARE(ui_img_qwe_png);       // 警告灯
LV_IMG_DECLARE(ui_img_shuiwen_png);   // 水温
LV_IMG_DECLARE(ui_img_youliang_png);  // 油量

void ui_landscape_init(void)
{
    // ========== 屏幕 ==========
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui_Screen1, 255, 0);

    // 左转向灯 - 左上角
    ui_left = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_left, &ui_img_left_png);
    lv_obj_set_pos(ui_left, 15, 8);

    // 中间图标组（整体居中）
    ui_sc = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_sc, &ui_img_sc_png);
    lv_obj_set_pos(ui_sc, 171, 5);

    ui_aqd = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_aqd, &ui_img_qeq_png);
    lv_obj_set_pos(ui_aqd, 222, 8);

    ui_warn = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_warn, &ui_img_qwe_png);
    lv_obj_set_pos(ui_warn, 273, 6);

    // 右转向灯 - 右上角
    ui_right = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_right, &ui_img_right_png);
    lv_obj_set_pos(ui_right, 432, 8);

    // ========== 转速表（左侧） ==========
    ui_mater_zhuansu = lv_meter_create(ui_Screen1);
    lv_obj_set_size(ui_mater_zhuansu, 180, 180);
    lv_obj_set_pos(ui_mater_zhuansu, 40, 30);
    lv_obj_set_style_bg_color(ui_mater_zhuansu, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui_mater_zhuansu, 255, 0);
    lv_obj_set_style_border_opa(ui_mater_zhuansu, 0, 0);

    // 设置仪表文字样式（关键！控制刻度标签字体和颜色）
    lv_obj_set_style_text_color(ui_mater_zhuansu, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ui_mater_zhuansu, &lv_font_montserrat_12, 0);

    lv_meter_scale_t * scale_zhuansu = lv_meter_add_scale(ui_mater_zhuansu);
    lv_meter_set_scale_range(ui_mater_zhuansu, scale_zhuansu, 0, 80, 270, 135);
    lv_meter_set_scale_ticks(ui_mater_zhuansu, scale_zhuansu, 41, 1, 10, lv_palette_main(LV_PALETTE_GREY));

    // 使用自动标签！label_gap = -25（刻度外侧40px）
    lv_meter_set_scale_major_ticks(ui_mater_zhuansu, scale_zhuansu, 10, 2, 15, lv_palette_main(LV_PALETTE_RED), -25);

    indic_zuansu = lv_meter_add_needle_line(ui_mater_zhuansu, scale_zhuansu, 3, lv_palette_main(LV_PALETTE_RED), -25);

    // 单位（放在仪表中心下方，不重叠）
    lv_obj_t * lbl_unit_rpm = lv_label_create(ui_Screen1);
    lv_label_set_text(lbl_unit_rpm, "x1000/min");
    lv_obj_set_style_text_color(lbl_unit_rpm, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_unit_rpm, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(lbl_unit_rpm, 85, 140);

    // 转速数字
    ui_label_rpm = lv_label_create(ui_Screen1);
    lv_label_set_text(ui_label_rpm, "0");
    lv_obj_set_style_text_color(ui_label_rpm, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ui_label_rpm, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ui_label_rpm, 100, 215);

    // ========== 速度表（右侧） ==========
    ui_mater_speed = lv_meter_create(ui_Screen1);
    lv_obj_set_size(ui_mater_speed, 180, 180);
    lv_obj_set_pos(ui_mater_speed, 260, 30);
    lv_obj_set_style_bg_color(ui_mater_speed, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui_mater_speed, 255, 0);
    lv_obj_set_style_border_opa(ui_mater_speed, 0, 0);

    // 设置仪表文字样式
    lv_obj_set_style_text_color(ui_mater_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ui_mater_speed, &lv_font_montserrat_12, 0);

    lv_meter_scale_t * scale_speed = lv_meter_add_scale(ui_mater_speed);
    lv_meter_set_scale_range(ui_mater_speed, scale_speed, 0, 240, 270, 135);
    lv_meter_set_scale_ticks(ui_mater_speed, scale_speed, 121, 1, 10, lv_palette_main(LV_PALETTE_GREY));

   // 使用自动标签！label_gap = -25
    lv_meter_set_scale_major_ticks(ui_mater_speed, scale_speed, 12, 2, 15, lv_palette_main(LV_PALETTE_RED), -25);

    indic_speed = lv_meter_add_needle_line(ui_mater_speed, scale_speed, 3, lv_color_hex(0xFFFFFF), -25);

    // 单位
    lv_obj_t * lbl_unit_speed = lv_label_create(ui_Screen1);
    lv_label_set_text(lbl_unit_speed, "km/h");
    lv_obj_set_style_text_color(lbl_unit_speed, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_unit_speed, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(lbl_unit_speed, 325, 140);

    // 速度数字
    ui_label_speed = lv_label_create(ui_Screen1);
    lv_label_set_text(ui_label_speed, "0");
    lv_obj_set_style_text_color(ui_label_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ui_label_speed, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ui_label_speed, 320, 215);

    // ========== 底部温度条 ==========
    lv_obj_t * lbl_temp_title = lv_label_create(ui_Screen1);
    lv_label_set_text(lbl_temp_title, "TEMP");
    lv_obj_set_style_text_color(lbl_temp_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_temp_title, 10, 260);

    ui_Bar1 = lv_bar_create(ui_Screen1);
    lv_obj_set_size(ui_Bar1, 180, 10);
    lv_obj_set_pos(ui_Bar1, 10, 285);
    lv_bar_set_range(ui_Bar1, 0, 150);
    lv_obj_set_style_bg_color(ui_Bar1, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_Bar1, lv_color_hex(0x00AA00), LV_PART_INDICATOR);

    ui_lbl_tmp = lv_label_create(ui_Screen1);
    lv_label_set_text(ui_lbl_tmp, "85 C");
    lv_obj_set_style_text_color(ui_lbl_tmp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(ui_lbl_tmp, 200, 280);

    // ========== 底部电量条 ==========
    lv_obj_t * lbl_bat_title = lv_label_create(ui_Screen1);
    lv_label_set_text(lbl_bat_title, "BATTERY");
    lv_obj_set_style_text_color(lbl_bat_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_bat_title, 250, 260);

    ui_bar_elc = lv_bar_create(ui_Screen1);
    lv_obj_set_size(ui_bar_elc, 180, 10);
    lv_obj_set_pos(ui_bar_elc, 250, 285);
    lv_bar_set_range(ui_bar_elc, 0, 100);
    lv_obj_set_style_bg_color(ui_bar_elc, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_bar_elc, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);

    ui_lbl_elc = lv_label_create(ui_Screen1);
    lv_label_set_text(ui_lbl_elc, "75%");
    lv_obj_set_style_text_color(ui_lbl_elc, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(ui_lbl_elc, 440, 280);

    lv_disp_load_scr(ui_Screen1);
}

void ui_set_zuansu(int zuansu)
{
    int z = zuansu / 100;
    if(z > 80) z = 80;
    lv_meter_set_indicator_value(ui_mater_zhuansu, indic_zuansu, z);
    lv_label_set_text_fmt(ui_label_rpm, "%d", zuansu);
}

void ui_set_speed(int speed)
{
    int s = speed;
    if(s > 240) s = 240;
    lv_meter_set_indicator_value(ui_mater_speed, indic_speed, s);
    lv_label_set_text_fmt(ui_label_speed, "%d", speed);
}

void ui_set_tmp(int temp)
{
    lv_bar_set_value(ui_Bar1, temp, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_lbl_tmp, "%d C", temp);
}

void ui_set_elc(int elc)
{
    lv_bar_set_value(ui_bar_elc, elc, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_lbl_elc, "%d%%", elc);
}

void ui_anquandai(void)
{
    if(visible) {
        lv_obj_add_flag(ui_aqd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_warn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_sc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_right, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(ui_aqd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_warn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_sc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_right, LV_OBJ_FLAG_HIDDEN);
    }
    visible = !visible;
}