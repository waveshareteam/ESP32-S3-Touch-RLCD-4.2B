#include "weather_init.h"
#include "weather_ui.h"
#include <string.h>

static weather_ui_t ui;
static lv_timer_t *timer;
static bool previous_valid;
static weather_snapshot_t previous_weather;
static wifi_manager_snapshot_t previous_network;

static void update(lv_timer_t *unused)
{
    (void)unused;
    if(!ui.screen) return;
    weather_snapshot_t weather;
    wifi_manager_snapshot_t network;
    weather_service_get_snapshot(&weather);
    wifi_manager_get_snapshot(&network);
    if (!previous_valid || memcmp(&weather,&previous_weather,sizeof(weather)) || memcmp(&network,&previous_network,sizeof(network))) {
        weather_ui_update(&ui,&weather,&network);
        previous_weather=weather; previous_network=network; previous_valid=true;
    }
}

static void request_refresh(void *context) { (void)context; weather_service_request_refresh(); }

static void on_delete(lv_event_t *event)
{
    if (lv_event_get_target(event)!=ui.screen) return;
    if(timer) { lv_timer_delete(timer); timer=NULL; }
    memset(&ui,0,sizeof(ui));
    previous_valid=false;
}

bool init_weather_ui_screen(lv_obj_t *parent,int32_t x,int32_t y,weather_back_cb_t back,void *context)
{
    if(!parent) return false;
    deinit_weather_ui();
    weather_ui_create(&ui,parent,x,y,back,request_refresh,context);
    lv_obj_set_user_data(ui.screen,&ui);
    lv_obj_add_event_cb(ui.screen,on_delete,LV_EVENT_DELETE,NULL);
    previous_valid=false;
    timer=lv_timer_create(update,500,NULL);
    if(!timer) { deinit_weather_ui(); return false; }
    update(NULL);
    return true;
}

void deinit_weather_ui(void) { if(ui.screen) lv_obj_delete(ui.screen); }
void pause_weather_ui(bool paused)
{
    if(!timer) return;
    if(paused) lv_timer_pause(timer);
    else { update(NULL); lv_timer_resume(timer); }
}
