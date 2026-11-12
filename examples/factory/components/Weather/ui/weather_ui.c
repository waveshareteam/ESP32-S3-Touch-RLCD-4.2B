#include "weather_ui.h"
#include "weather_icons.h"
#include "rlcd_app_header.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *box(lv_obj_t *parent,int x,int y,int width,int height,bool black)
{
    lv_obj_t *obj=lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(obj,x,y); lv_obj_set_size(obj,width,height);
    lv_obj_set_style_bg_color(obj,black?lv_color_black():lv_color_white(),0);
    lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent,const char *text,int x,int y,int width,const lv_font_t *font,lv_text_align_t align)
{
    lv_obj_t *obj=lv_label_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_text_font(obj,font,0);
    lv_obj_set_style_text_color(obj,lv_color_black(),0);
    lv_obj_set_style_text_align(obj,align,0);
    lv_obj_set_width(obj,width); lv_obj_set_pos(obj,x,y);
    lv_label_set_long_mode(obj,LV_LABEL_LONG_DOT);
    lv_label_set_text(obj,text);
    return obj;
}

static void set_text(lv_obj_t *obj,const char *text)
{
    if (strcmp(lv_label_get_text(obj),text)) lv_label_set_text(obj,text);
}

static void back_event(lv_event_t *event)
{
    weather_ui_t *ui=lv_event_get_user_data(event);
    if(ui->back) ui->back(ui->context);
}

static void refresh_event(lv_event_t *event)
{
    weather_ui_t *ui=lv_event_get_user_data(event);
    if(ui->refresh) ui->refresh(ui->context);
}

void weather_ui_create(weather_ui_t *ui,lv_obj_t *parent,int32_t x,int32_t y,
                        weather_action_cb_t back,weather_action_cb_t refresh,void *context)
{
    memset(ui,0,sizeof(*ui));
    ui->back=back; ui->refresh=refresh; ui->context=context;
    ui->screen=box(parent,x,y,400,300,false);
    /* Keep touches within the app's opaque screen. */
    lv_obj_add_flag(ui->screen,LV_OBJ_FLAG_CLICKABLE);
    rlcd_app_header_t header=rlcd_app_header_create(ui->screen,"Weather",&rlcd_header_weather,back_event,ui);
    ui->back_button=header.back;
    ui->screen_data_lab=label(ui->screen,"--",252,54,132,&lv_font_montserrat_14,LV_TEXT_ALIGN_RIGHT);
    ui->screen_diqu_lab=label(ui->screen,"",20,70,210,&lv_font_montserrat_20,LV_TEXT_ALIGN_LEFT);
    ui->screen_wendu_lab=label(ui->screen,"--",16,103,162,&lv_font_montserrat_48,LV_TEXT_ALIGN_LEFT);
    lv_obj_set_width(ui->screen_wendu_lab,LV_SIZE_CONTENT);
    ui->temperature_unit=label(ui->screen,"\xc2\xb0" "C",128,113,45,&lv_font_montserrat_24,LV_TEXT_ALIGN_LEFT);
    ui->screen_weather_lab=label(ui->screen,"Waiting for data",20,166,205,&lv_font_montserrat_16,LV_TEXT_ALIGN_LEFT);
    ui->screen_weather_img=lv_image_create(ui->screen);
    lv_image_set_src(ui->screen_weather_img,weather_icon_get(WEATHER_UNKNOWN));
    lv_obj_set_pos(ui->screen_weather_img,232,75);
    lv_obj_add_flag(ui->screen_weather_img,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->screen_weather_img,refresh_event,LV_EVENT_CLICKED,ui);
    ui->status_label=label(ui->screen,"",20,187,364,&lv_font_montserrat_12,LV_TEXT_ALIGN_LEFT);
    box(ui->screen,16,204,368,1,true);
    box(ui->screen,139,222,1,55,true); box(ui->screen,260,222,1,55,true);
    label(ui->screen,"HUMIDITY",16,222,123,&lv_font_montserrat_14,LV_TEXT_ALIGN_CENTER);
    label(ui->screen,"WIND SPEED",140,222,120,&lv_font_montserrat_14,LV_TEXT_ALIGN_CENTER);
    label(ui->screen,"DIRECTION",261,222,123,&lv_font_montserrat_14,LV_TEXT_ALIGN_CENTER);
    ui->screen_shidu_lab=label(ui->screen,"--",16,248,123,&lv_font_montserrat_24,LV_TEXT_ALIGN_CENTER);
    ui->screen_fengsu_lab=label(ui->screen,"--",140,248,120,&lv_font_montserrat_24,LV_TEXT_ALIGN_CENTER);
    ui->screen_fengxiang_lab=label(ui->screen,"--",261,248,123,&lv_font_montserrat_24,LV_TEXT_ALIGN_CENTER);

}

void weather_ui_update(weather_ui_t *ui,const weather_snapshot_t *weather,const wifi_manager_snapshot_t *network)
{
    if (!ui || !ui->screen) return;
    const weather_data_t *data=&weather->data;
    set_text(ui->screen_diqu_lab,data->city[0]?data->city:"Shenzhen");
    if(weather->cached) {
        set_text(ui->screen_data_lab,data->date);
        set_text(ui->screen_weather_lab,data->weather);
        set_text(ui->screen_wendu_lab,data->temp);
        char humidity[20];
        snprintf(humidity,sizeof(humidity),strcmp(data->humi,"--")?"%s%%":"%s",data->humi);
        set_text(ui->screen_shidu_lab,humidity);
        set_text(ui->screen_fengsu_lab,data->wind_speed);
        set_text(ui->screen_fengxiang_lab,data->wind_dir);
    }
    const lv_image_dsc_t *icon=weather_icon_get(weather->cached?data->condition:WEATHER_UNKNOWN);
    if (lv_image_get_src(ui->screen_weather_img)!=icon) lv_image_set_src(ui->screen_weather_img,icon);
    lv_obj_update_layout(ui->screen_wendu_lab);
    lv_obj_set_x(ui->temperature_unit,16+lv_obj_get_width(ui->screen_wendu_lab)+8);
    const char *status=weather->message;
    if (!network->ready) status=network->message[0]?network->message:"Starting Wi-Fi...";
    else if (!network->connected && weather->cached) status="Offline - showing last weather.";
    else if (!network->connected) status=network->message[0]?network->message:"Waiting for phone hotspot...";
    else if (weather->state==WEATHER_ERROR && weather->cached) status="Update failed - showing last weather.";
    set_text(ui->status_label,status);
}
