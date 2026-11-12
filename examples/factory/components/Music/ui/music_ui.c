#include "music_ui.h"
#include "music_service.h"
#include "music_assets.h"
#include "rlcd_app_header.h"
#include <string.h>
#include <stdio.h>

static struct {
    lv_obj_t *root, *title, *detail, *count, *bar, *elapsed, *duration;
    lv_obj_t *previous, *play, *next, *play_symbol;
    lv_timer_t *timer;
    music_back_fn back;
    void *context;
    int last_percent;
} ui;

static lv_obj_t *box(lv_obj_t *parent,int x,int y,int w,int h)
{
    lv_obj_t *obj=lv_obj_create(parent); lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj,x,y); lv_obj_set_size(obj,w,h);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(obj,lv_color_white(),0); lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
    return obj;
}
static lv_obj_t *label(lv_obj_t *parent,const char *text,int x,int y,int w,const lv_font_t *font)
{
    lv_obj_t *obj=lv_label_create(parent); lv_obj_set_pos(obj,x,y); lv_obj_set_width(obj,w);
    lv_obj_set_style_text_font(obj,font,0); lv_obj_set_style_text_color(obj,lv_color_black(),0);
    lv_label_set_long_mode(obj,LV_LABEL_LONG_DOT); lv_label_set_text(obj,text);
    return obj;
}
static void set_text(lv_obj_t *obj,const char *text)
{ if (strcmp(lv_label_get_text(obj),text)) lv_label_set_text(obj,text); }
static void click(lv_event_t *e)
{
    lv_obj_t *target=lv_event_get_target(e);
    if (target==ui.play) music_service_toggle();
    else music_service_step(target==ui.previous ? -1 : 1);
}
static lv_obj_t *button(int x,int y,int size,const char *symbol,bool main)
{
    lv_obj_t *obj=box(ui.root,x,y,size,size);
    lv_obj_add_flag(obj,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(obj,LV_RADIUS_CIRCLE,0);
    lv_obj_set_style_bg_color(obj,main?lv_color_black():lv_color_white(),0);
    lv_obj_set_style_border_color(obj,lv_color_black(),0);
    lv_obj_set_style_border_width(obj,2,LV_STATE_PRESSED);
    lv_obj_t *caption=label(obj,symbol,0,0,size,main?&lv_font_montserrat_32:&lv_font_montserrat_40);
    lv_obj_set_style_text_color(caption,main?lv_color_white():lv_color_black(),0);
    lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0); lv_obj_center(caption);
    lv_obj_add_event_cb(obj,click,LV_EVENT_CLICKED,NULL);
    if (main) ui.play_symbol=caption;
    return obj;
}
static void enable(lv_obj_t *obj,bool enabled)
{
    if(enabled) lv_obj_remove_state(obj,LV_STATE_DISABLED);
    else lv_obj_add_state(obj,LV_STATE_DISABLED);
}
static void poll(lv_timer_t *unused)
{
    (void)unused;
    music_snapshot_t state; music_service_snapshot(&state);
    char text[32];
    snprintf(text,sizeof(text),"%02u / %02u",(unsigned)(state.count?state.index+1:0),(unsigned)state.count);
    set_text(ui.count,text);
    set_text(ui.title,state.status==MUSIC_UI_READY?"Music Player":state.status==MUSIC_UI_LOADING?"Loading...":
        state.status==MUSIC_UI_NO_CARD?"No SD card":state.status==MUSIC_UI_EMPTY?"No music":
        state.status==MUSIC_UI_NO_FOLDER?"No music folder":"Music");
    set_text(ui.detail,state.status==MUSIC_UI_READY?"Chill Vibes":state.detail);
    set_text(ui.play_symbol,state.playing?LV_SYMBOL_PAUSE:LV_SYMBOL_PLAY);
    enable(ui.previous,state.count>0); enable(ui.next,state.count>0);
    enable(ui.play,state.status!=MUSIC_UI_LOADING);
    music_format_time(state.position_ms,text,sizeof(text)); set_text(ui.elapsed,text);
    if(state.duration_ms) music_format_time(state.duration_ms,text,sizeof(text));
    else snprintf(text,sizeof(text),"--:--");
    set_text(ui.duration,text);
    /* Round the display position to whole seconds, avoiding unnecessary RLCD refreshes. */
    uint32_t display_ms=state.position_ms/1000*1000;
    if(state.duration_ms && state.position_ms>=state.duration_ms) display_ms=state.duration_ms;
    int percent=state.duration_ms?(int)((uint64_t)display_ms*1000/state.duration_ms):0;
    if(percent>1000) percent=1000;
    if(percent!=ui.last_percent) {
        lv_bar_set_value(ui.bar,percent,LV_ANIM_OFF);
        ui.last_percent=percent;
    }
}
static void gesture(lv_event_t *event)
{
    if(lv_event_get_target(event)!=ui.root) return;
    lv_indev_t *touch=lv_indev_active();
    if(touch && lv_indev_get_gesture_dir(touch)==LV_DIR_TOP) {
        lv_indev_wait_release(touch);
        if(ui.back) ui.back(ui.context);
    }
}
static void back_event(lv_event_t *event)
{
    (void)event;
    if(ui.back) ui.back(ui.context);
}
static void deleted(lv_event_t *event)
{
    if(lv_event_get_target(event)!=ui.root) return;
    if(ui.timer) lv_timer_delete(ui.timer);
    music_service_close(); memset(&ui,0,sizeof(ui));
}
bool music_ui_init(lv_obj_t *parent,int x,int y,music_back_fn back,void *ctx)
{
    if(!parent) return false;
    music_ui_deinit();
    if(!music_service_open()) return false;
    ui.back=back; ui.context=ctx; ui.last_percent=-1;
    ui.root=box(parent,x,y,400,300);
    lv_obj_add_flag(ui.root,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(ui.root,LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(ui.root,gesture,LV_EVENT_GESTURE,NULL);
    lv_obj_add_event_cb(ui.root,deleted,LV_EVENT_DELETE,NULL);
    rlcd_app_header_create(ui.root,"Music",&rlcd_header_music,back_event,NULL);
    ui.count=label(ui.root,"00 / 00",278,58,102,&lv_font_montserrat_16);
    lv_obj_set_style_text_align(ui.count,LV_TEXT_ALIGN_RIGHT,0);
    lv_obj_t *cover=lv_image_create(ui.root); lv_image_set_src(cover,&music_cover); lv_obj_set_pos(cover,20,58);
    ui.title=label(ui.root,"Loading...",178,94,203,&lv_font_montserrat_26); lv_obj_set_height(ui.title,32);
    ui.detail=label(ui.root,"",178,134,203,&lv_font_montserrat_16); lv_obj_set_height(ui.detail,52);
    /* A one-pixel track and solid played segment stay crisp on the I1 panel.
     * This is a read-only indicator, so there is no draggable-looking thumb. */
    lv_obj_t *track=box(ui.root,20,206,360,1);
    lv_obj_set_style_bg_color(track,lv_color_black(),0);
    ui.bar=lv_bar_create(ui.root); lv_obj_remove_style_all(ui.bar);
    lv_obj_set_pos(ui.bar,20,204); lv_obj_set_size(ui.bar,360,4); lv_bar_set_range(ui.bar,0,1000);
    lv_obj_set_style_bg_color(ui.bar,lv_color_black(),LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ui.bar,LV_OPA_COVER,LV_PART_INDICATOR); lv_obj_set_style_radius(ui.bar,2,LV_PART_INDICATOR);
    lv_obj_remove_flag(ui.bar,LV_OBJ_FLAG_CLICKABLE);
    ui.elapsed=label(ui.root,"00:00",20,216,90,&lv_font_montserrat_14);
    ui.duration=label(ui.root,"--:--",290,216,90,&lv_font_montserrat_14);
    lv_obj_set_style_text_align(ui.duration,LV_TEXT_ALIGN_RIGHT,0);
    ui.previous=button(96,231,52,LV_SYMBOL_PREV,false);
    ui.play=button(170,224,60,LV_SYMBOL_PLAY,true);
    ui.next=button(252,231,52,LV_SYMBOL_NEXT,false);
    ui.timer=lv_timer_create(poll,200,NULL);
    if(!ui.timer) { music_ui_deinit(); return false; }
    poll(NULL); return true;
}
void music_ui_deinit(void) { if(ui.root) lv_obj_delete(ui.root); }
void music_ui_pause(bool paused)
{
    if(!ui.timer) return;
    music_service_suspend(paused);
    if(paused) lv_timer_pause(ui.timer);
    else { poll(NULL); lv_timer_resume(ui.timer); }
}
