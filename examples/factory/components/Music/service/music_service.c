#include "music_service.h"
#include "bsp/esp-bsp.h"
#include "esp_audio_simple_player.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>

/* File scanning and data conversion shared by the worker and UI. */
void music_files_free(music_files_t *files)
{
    for (size_t i = 0; i < files->count; ++i) free(files->paths[i]);
    free(files->paths); *files = (music_files_t){0};
}
static int compare(const void *a, const void *b)
{
    const char *x = *(const char *const *)a, *y = *(const char *const *)b;
    int result = strcasecmp(x, y);
    return result ? result : strcmp(x, y);
}
music_result_t music_scan(const char *folder, music_files_t *files, music_cancel_fn cancel, void *ctx)
{
    *files = (music_files_t){0};
    DIR *dir = opendir(folder);
    if (!dir) return errno == ENOENT ? MUSIC_NO_FOLDER : MUSIC_IO_ERROR;
    music_result_t result = MUSIC_OK;
    size_t capacity = 0;
    for (;;) {
        if (cancel && cancel(ctx)) { result = MUSIC_CANCELLED; break; }
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (!entry) { if (errno) result = MUSIC_IO_ERROR; break; }
        const char *ext = strrchr(entry->d_name, '.');
        if (entry->d_name[0] == '.' || !ext || strcasecmp(ext, ".mp3")) continue;
        char path[MUSIC_PATH_MAX];
        int length = snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
        if (length < 0 || (size_t)length >= sizeof(path)) continue;
        struct stat st;
        if (stat(path, &st)) { result = MUSIC_IO_ERROR; break; }
        if (!S_ISREG(st.st_mode)) continue;
        if (files->count == capacity) {
            size_t next = capacity ? capacity * 2 : 16;
            char **grown = realloc(files->paths, next * sizeof(*grown));
            if (!grown) { result = MUSIC_NO_MEMORY; break; }
            files->paths = grown; capacity = next;
        }
        char *copy = malloc((size_t)length + 1);
        if (!copy) { result = MUSIC_NO_MEMORY; break; }
        memcpy(copy, path, (size_t)length + 1);
        files->paths[files->count++] = copy;
    }
    closedir(dir);
    if (result == MUSIC_OK && !files->count) result = MUSIC_EMPTY;
    if (result != MUSIC_OK) music_files_free(files);
    else qsort(files->paths, files->count, sizeof(*files->paths), compare);
    return result;
}
void music_title(const char *path, char *title, size_t capacity)
{
    const char *name = strrchr(path, '/'); name = name ? name + 1 : path;
    size_t length = strlen(name);
    if (length >= 4 && !strcasecmp(name + length - 4, ".mp3")) length -= 4;
    if (!capacity) return;
    if (length >= capacity) length = capacity - 1;
    /* Avoid leaving a truncated UTF-8 character at the end. Font fallback is UI-owned. */
    while (length && ((unsigned char)name[length] & 0xc0) == 0x80) --length;
    memcpy(title, name, length); title[length] = 0;
}
void music_format_time(uint32_t ms, char *out, size_t capacity)
{
    snprintf(out, capacity, "%02lu:%02lu", (unsigned long)(ms / 60000), (unsigned long)(ms / 1000 % 60));
}
static uint32_t be32(const unsigned char *p)
{ return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
typedef struct { unsigned size, rate, samples, version, channels, crc; } frame_t;
static uint32_t syncsafe(const unsigned char *p)
{ return ((uint32_t)(p[0]&127)<<21)|((uint32_t)(p[1]&127)<<14)|((uint32_t)(p[2]&127)<<7)|(p[3]&127); }
static void tag_text(const unsigned char *p,size_t n,char *out,size_t capacity)
{
    if(n<2 || !capacity) return;
    unsigned encoding=*p++; --n;
    if(encoding==0 || encoding==3) {
        size_t size=0;
        for(size_t i=0;i<n && p[i];++i) {
            unsigned c=p[i];
            if(c<32) c=' ';
            if(encoding==0 && c>=128) {
                if(size+2>=capacity) break;
                out[size++]=(char)(0xc0|(c>>6)); out[size++]=(char)(0x80|(c&63));
            } else { if(size+1>=capacity) break; out[size++]=(char)c; }
        }
        if(encoding==3 && size<n) while(size && (p[size]&0xc0)==0x80) --size;
        out[size]=0; return;
    }
    if(encoding!=1 && encoding!=2) return;
    bool little=false;
    if(n>=2 && ((p[0]==0xff && p[1]==0xfe)||(p[0]==0xfe && p[1]==0xff))) {
        little=p[0]==0xff; p+=2; n-=2;
    }
    size_t used=0;
    for(size_t i=0;i+1<n;i+=2) {
        uint32_t cp=little ? p[i]|((uint32_t)p[i+1]<<8) : ((uint32_t)p[i]<<8)|p[i+1];
        if(!cp) break;
        if(cp>=0xd800 && cp<=0xdbff && i+3<n) {
            uint32_t lo=little ? p[i+2]|((uint32_t)p[i+3]<<8) : ((uint32_t)p[i+2]<<8)|p[i+3];
            if(lo>=0xdc00 && lo<=0xdfff) { cp=0x10000+((cp-0xd800)<<10)+lo-0xdc00; i+=2; }
        }
        if(cp>=0xd800 && cp<=0xdfff) cp='?';
        if(cp<32) cp=' ';
        unsigned count=cp<128?1:cp<2048?2:cp<65536?3:4;
        if(used+count>=capacity) break;
        if(count==1) out[used++]=(char)cp;
        else {
            out[used++]=(char)((count==2?0xc0:count==3?0xe0:0xf0)|(cp>>(6*(count-1))));
            for(int shift=(int)(count-2)*6;shift>=0;shift-=6) out[used++]=(char)(0x80|((cp>>shift)&63));
        }
    }
    out[used]=0;
}
static void read_tags(FILE *file,const unsigned char *id3,long end,music_info_t *info)
{
    unsigned version=id3[3];
    /* Uncommon compressed/unsynchronised tags are optional metadata: skip them,
     * without rejecting the underlying MP3 or trying to read cover pictures. */
    if((version!=3 && version!=4) || (id3[5]&0xc0)) return;
    long pos=10;
    while(pos+10<=end) {
        unsigned char head[10],text[512];
        if(fseek(file,pos,SEEK_SET) || fread(head,1,10,file)!=10 || !head[0]) return;
        uint32_t size=version==4?syncsafe(head+4):be32(head+4);
        if(!size || size>(uint32_t)(end-pos-10)) return;
        bool title=!memcmp(head,"TIT2",4),artist=!memcmp(head,"TPE1",4);
        if((title||artist) && head[9]==0) {
            size_t read=size<sizeof(text)?size:sizeof(text);
            if(fread(text,1,read,file)!=read) return;
            tag_text(text,read,title?info->title:info->artist,title?sizeof(info->title):sizeof(info->artist));
        }
        pos+=10+(long)size;
        if(info->title[0] && info->artist[0]) return;
    }
}
static bool header(const unsigned char *p, frame_t *f)
{
    static const unsigned rates[] = {44100,48000,32000};
    static const unsigned m1[] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
    static const unsigned m2[] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160};
    unsigned ver = (p[1] >> 3) & 3, layer = (p[1] >> 1) & 3;
    unsigned bitrate = p[2] >> 4, rate = (p[2] >> 2) & 3;
    if (p[0] != 0xff || (p[1] & 0xe0) != 0xe0 || ver == 1 || layer != 1 ||
        !bitrate || bitrate == 15 || rate == 3) return false;
    f->rate = rates[rate] / (ver == 3 ? 1 : ver == 2 ? 2 : 4);
    f->samples = ver == 3 ? 1152 : 576;
    f->size = (ver == 3 ? 144000 : 72000) * (ver == 3 ? m1[bitrate] : m2[bitrate]) / f->rate + ((p[2] >> 1) & 1);
    f->version = ver; f->channels = (p[3] >> 6) == 3 ? 1 : 2; f->crc = (p[1] & 1) ? 0 : 2;
    return f->size >= 4 && f->size <= 2048;
}
music_result_t music_probe(const char *path, music_info_t *info, music_cancel_fn cancel, void *ctx)
{
    *info = (music_info_t){0};
    FILE *file = fopen(path, "rb");
    if (!file) return MUSIC_IO_ERROR;
    setvbuf(file, NULL, _IOFBF, 8192);
    unsigned char data[2048];
    music_result_t result = MUSIC_BAD_MP3;
    if (fread(data, 1, 10, file) != 10) goto done;
    long offset = 0;
    if (!memcmp(data, "ID3", 3)) {
        if ((data[6] | data[7] | data[8] | data[9]) & 0x80) goto done;
        offset = 10 + ((long)data[6]<<21) + ((long)data[7]<<14) + ((long)data[8]<<7) + data[9];
        read_tags(file,data,offset,info);
        if (data[3] == 4 && (data[5] & 0x10)) offset += 10;
    }
    if (fseek(file, offset, SEEK_SET)) { result = MUSIC_IO_ERROR; goto done; }
    uint64_t samples = 0;
    unsigned frames = 0, searched = 0, first_rate = 0, first_version = 0;
    if (fread(data, 1, 4, file) != 4) goto done;
    for (;;) {
        if (cancel && cancel(ctx)) { result = MUSIC_CANCELLED; goto done; }
        frame_t f;
        if (!header(data, &f)) {
            if (frames) {
                /* ID3v1/APE trailers are allowed; garbage inside audio is not. */
                if (!memcmp(data, "TAG", 3) || !memcmp(data, "APET", 4)) break;
                goto done;
            }
            if (++searched > 65536) goto done;
            memmove(data, data + 1, 3);
            int c = fgetc(file); if (c == EOF) goto done;
            data[3] = (unsigned char)c; continue;
        }
        if (frames && (f.rate != first_rate || f.version != first_version)) goto done;
        if (fread(data + 4, 1, f.size - 4, file) != f.size - 4) goto done;
        if (!frames) {
            first_rate = f.rate; first_version = f.version;
            info->sample_rate = f.rate; info->channels = f.channels;
            unsigned side = f.version == 3 ? (f.channels == 1 ? 17 : 32) : (f.channels == 1 ? 9 : 17);
            unsigned pos = 4 + f.crc + side;
            uint32_t count = 0;
            if (pos + 12 <= f.size && (!memcmp(data+pos,"Xing",4) || !memcmp(data+pos,"Info",4)) &&
                (be32(data+pos+4) & 1)) count = be32(data+pos+8);
            if (!count && f.size >= 54 && !memcmp(data+36,"VBRI",4)) count = be32(data+50);
            if (count) {
                uint64_t duration = (uint64_t)count * f.samples * 1000 / f.rate;
                if (duration && duration <= UINT32_MAX) {
                    info->duration_ms = (uint32_t)duration; result = MUSIC_OK; goto done;
                }
            }
        }
        ++frames; samples += f.samples;
        size_t got = fread(data, 1, 4, file);
        if (!got) break;
        if (got != 4) goto done;
    }
    if (frames && samples * 1000 / first_rate <= UINT32_MAX) {
        info->duration_ms = (uint32_t)(samples * 1000 / first_rate); result = MUSIC_OK;
    }
done:
    if (ferror(file)) result = MUSIC_IO_ERROR;
    fclose(file);
    if (result != MUSIC_OK) *info = (music_info_t){0};
    return result;
}

/* Background service and hardware access. */
_Static_assert(CONFIG_AUDIO_SIMPLE_PLAYER_RESAMPLE_DEST_RATE == BSP_AUDIO_PLAYBACK_RATE, "PCM rate must match BSP");
_Static_assert(CONFIG_AUDIO_SIMPLE_PLAYER_CH_CVT_DEST == BSP_AUDIO_PLAYBACK_CHANNELS, "PCM channels must match BSP");

typedef struct { uint32_t generation, selection; size_t index; bool active, play, suspended; } request_t;
static const char *TAG = "music";
static SemaphoreHandle_t lock;
static TaskHandle_t task;
static request_t request;
static music_snapshot_t snapshot;
/* Decoder callback data; FILE is closed only after the decoder task joins. */
static FILE *input;
static uint64_t pcm_bytes;
static bool io_error;
static esp_asp_state_t audio_state;
static portMUX_TYPE audio_lock = portMUX_INITIALIZER_UNLOCKED;

static request_t get_request(void)
{
    xSemaphoreTake(lock, portMAX_DELAY); request_t value = request; xSemaphoreGive(lock); return value;
}
static bool cancelled(void *ctx)
{
    const request_t *job = ctx; request_t current = get_request();
    return !current.active || current.generation != job->generation || current.selection != job->selection;
}
static void publish(const request_t *job, const music_snapshot_t *value)
{
    xSemaphoreTake(lock, portMAX_DELAY);
    if (request.active && request.generation == job->generation && request.selection == job->selection) snapshot = *value;
    xSemaphoreGive(lock);
}
static void clear_play(const request_t *job)
{
    xSemaphoreTake(lock, portMAX_DELAY);
    if (request.generation == job->generation && request.selection == job->selection) request.play = false;
    xSemaphoreGive(lock);
}
static int read_audio(uint8_t *data, int length, void *ctx)
{
    (void)ctx;
    size_t got = fread(data, 1, (size_t)length, input);
    if (ferror(input)) { portENTER_CRITICAL(&audio_lock); io_error = true; portEXIT_CRITICAL(&audio_lock); }
    return (int)got;
}
static int write_audio(uint8_t *data, int length, void *ctx)
{
    (void)ctx;
    esp_err_t err = bsp_audio_playback_write(data, (size_t)length);
    portENTER_CRITICAL(&audio_lock);
    if (err == ESP_OK) pcm_bytes += (unsigned)length;
    else io_error = true;
    portEXIT_CRITICAL(&audio_lock);
    return err == ESP_OK ? 0 : -1;
}
static int audio_event(esp_asp_event_pkt_t *event, void *ctx)
{
    (void)ctx;
    if (event->type == ESP_ASP_EVENT_TYPE_STATE && event->payload_size == sizeof(esp_asp_state_t)) {
        portENTER_CRITICAL(&audio_lock);
        memcpy(&audio_state, event->payload, sizeof(audio_state));
        portEXIT_CRITICAL(&audio_lock);
    }
    return 0;
}
static void reset_audio_stats(void)
{
    portENTER_CRITICAL(&audio_lock);
    pcm_bytes = 0; io_error = false; audio_state = ESP_ASP_STATE_NONE;
    portEXIT_CRITICAL(&audio_lock);
}
static void destroy_player(esp_asp_handle_t *player)
{
    bsp_audio_playback_mute(true);
    if (*player) {
        /* Stop also covers the asynchronous OPENING -> RUNNING transition;
         * only then may destroy unregister decoders and release FILE data. */
        esp_audio_simple_player_stop(*player);
        esp_audio_simple_player_destroy(*player); *player = NULL;
    }
    if (input) { fclose(input); input = NULL; }
}
static bool start_player(esp_asp_handle_t *player, const char *path)
{
    destroy_player(player);
    if (bsp_audio_playback_open() != ESP_OK) return false;
    input = fopen(path, "rb");
    if (!input) return false;
    setvbuf(input, NULL, _IOFBF, 8192);
    reset_audio_stats();
    esp_asp_cfg_t config = {
        .in = {.cb = read_audio}, .out = {.cb = write_audio},
        .task_prio = 5, .task_stack = 12288, .task_core = 1,
    };
    if (esp_audio_simple_player_new(&config, player) != ESP_GMF_ERR_OK) goto fail;
    if (esp_audio_simple_player_set_event(*player, audio_event, NULL) != ESP_GMF_ERR_OK) goto fail;
    /* Fixed raw URI avoids URL interpretation of spaces/#/UTF-8 in filenames.
     * The source callback still reads the selected SD MP3 as a stream. */
    if (bsp_audio_playback_mute(false) != ESP_OK ||
        esp_audio_simple_player_run(*player, "raw://sdcard/music.mp3", NULL) != ESP_GMF_ERR_OK) goto fail;
    ESP_LOGI(TAG, "Play %s", path);
    return true;
fail:
    destroy_player(player); return false;
}
static void set_error(music_snapshot_t *view, const char *message)
{
    view->status = MUSIC_UI_ERROR; view->playing = false;
    snprintf(view->detail, sizeof(view->detail), "%s", message);
}
static void worker(void *unused)
{
    (void)unused;
    request_t current = {0};
    music_files_t files = {0};
    music_snapshot_t view = {0};
    esp_asp_handle_t player = NULL;
    bool selected = false, started = false, paused = false, sd_lease = false;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
        request_t job = get_request();
        if (job.generation != current.generation) {
            destroy_player(&player); bsp_audio_playback_close();
            if (sd_lease) { bsp_sdcard_release(); sd_lease = false; }
            music_files_free(&files); selected = started = paused = false;
            current = job; view = (music_snapshot_t){.status = MUSIC_UI_LOADING};
            if (!job.active) continue;
            if (bsp_sdcard_acquire() != ESP_OK) {
                view.status = MUSIC_UI_NO_CARD;
                snprintf(view.detail,sizeof(view.detail),"Insert SD card; press Play to retry");
                publish(&job,&view); continue;
            }
            sd_lease = true;
            music_result_t result = music_scan(BSP_SD_MOUNT_POINT "/music", &files, cancelled, &job);
            if (cancelled(&job)) continue;
            if (result != MUSIC_OK) {
                view.status = result == MUSIC_NO_FOLDER ? MUSIC_UI_NO_FOLDER : result == MUSIC_EMPTY ? MUSIC_UI_EMPTY : MUSIC_UI_ERROR;
                snprintf(view.detail,sizeof(view.detail),"%s", result == MUSIC_NO_FOLDER ? "Create /music on the SD card" :
                    result == MUSIC_EMPTY ? "Copy MP3 files into /music" : "Cannot read music folder");
                publish(&job,&view); continue;
            }
            ESP_LOGI(TAG, "Found %u MP3 files in /music", (unsigned)files.count);
        }
        if (job.suspended) {
            destroy_player(&player);
            bsp_audio_playback_close();
            started = paused = false;
            view.playing = false;
            view.position_ms = 0;
            publish(&job, &view);
            continue;
        }
        if (!job.active || !files.count) continue;
        if (!selected || job.selection != current.selection) {
            destroy_player(&player); started = paused = false;
            current = job; selected = true;
            view = (music_snapshot_t){.status=MUSIC_UI_LOADING,.index=job.index,.count=files.count};
            music_title(files.paths[job.index],view.title,sizeof(view.title));
            snprintf(view.detail,sizeof(view.detail),"Reading MP3..."); publish(&job,&view);
            music_info_t info;
            music_result_t result = music_probe(files.paths[job.index],&info,cancelled,&job);
            if (cancelled(&job)) continue;
            if (result != MUSIC_OK) {
                set_error(&view, result == MUSIC_BAD_MP3 ? "Invalid MP3; select another track" : "SD read failed; select another track");
                clear_play(&job); publish(&job,&view); continue;
            }
            view.status = MUSIC_UI_READY; view.duration_ms = info.duration_ms;
            if (info.title[0]) snprintf(view.title,sizeof(view.title),"%s",info.title);
            snprintf(view.detail,sizeof(view.detail),"%s",info.artist[0] ? info.artist : "Local music");
            ESP_LOGI(TAG,"Track %u/%u: %s, duration=%lu ms",(unsigned)(job.index+1),(unsigned)files.count,
                     view.title,(unsigned long)info.duration_ms);
            job = get_request();
            if (cancelled(&current)) continue;
        }
        if (view.status == MUSIC_UI_ERROR) { publish(&job,&view); continue; }
        portENTER_CRITICAL(&audio_lock);
        esp_asp_state_t before = audio_state;
        portEXIT_CRITICAL(&audio_lock);
        bool ok = true;
        if (job.play && !started) {
            ok = start_player(&player,files.paths[job.index]);
            started = ok; paused = false; view.position_ms = 0;
        } else if (job.play && paused && before == ESP_ASP_STATE_PAUSED) {
            ok = bsp_audio_playback_mute(false) == ESP_OK && esp_audio_simple_player_resume(player) == ESP_GMF_ERR_OK;
            if (ok) paused = false;
        } else if (!job.play && started && !paused && before == ESP_ASP_STATE_RUNNING) {
            ok = esp_audio_simple_player_pause(player) == ESP_GMF_ERR_OK;
            if (ok) bsp_audio_playback_drain();
            bsp_audio_playback_mute(true);
            if (ok) paused = true;
        }
        portENTER_CRITICAL(&audio_lock);
        esp_asp_state_t state = audio_state;
        bool failed = io_error;
        uint64_t bytes = pcm_bytes;
        portEXIT_CRITICAL(&audio_lock);
        if (!ok || (started && (failed || state == ESP_ASP_STATE_ERROR))) {
            ESP_LOGE(TAG,"Playback failed for %s",files.paths[job.index]);
            destroy_player(&player); started = paused = false;
            set_error(&view,"Audio error; press Play to retry"); clear_play(&job);
        } else if (started && state == ESP_ASP_STATE_FINISHED) {
            bsp_audio_playback_drain();
            view.position_ms = view.duration_ms; view.playing = false;
            destroy_player(&player); started = paused = false; clear_play(&job);
        } else {
            view.playing = started && !paused && state == ESP_ASP_STATE_RUNNING;
            if (started) {
                uint64_t position = bytes * 1000 / (BSP_AUDIO_PLAYBACK_RATE * BSP_AUDIO_PLAYBACK_CHANNELS * (BSP_AUDIO_PLAYBACK_BITS / 8));
                view.position_ms = position > view.duration_ms ? view.duration_ms : (uint32_t)position;
            }
        }
        publish(&job,&view);
    }
}
bool music_service_open(void)
{
    /* Called by the LVGL task only; worker/decoder do not initialize this service. */
    if (!task) {
        lock = xSemaphoreCreateMutex();
        if (!lock) return false;
        if (xTaskCreate(worker,"rlcd_music",8192,NULL,4,&task) != pdPASS) {
            vSemaphoreDelete(lock); lock = NULL; return false;
        }
    }
    xSemaphoreTake(lock,portMAX_DELAY);
    ++request.generation; request.selection = 0; request.index = 0; request.active = true; request.play = false;
    request.suspended = false;
    snapshot = (music_snapshot_t){.status=MUSIC_UI_LOADING};
    snprintf(snapshot.detail,sizeof(snapshot.detail),"Reading SD card...");
    xSemaphoreGive(lock); xTaskNotifyGive(task); return true;
}
void music_service_close(void)
{
    if (!task) return;
    xSemaphoreTake(lock,portMAX_DELAY); ++request.generation; request.active = false; request.play = false;
    xSemaphoreGive(lock); xTaskNotifyGive(task);
}
void music_service_toggle(void)
{
    if (!task) return;
    xSemaphoreTake(lock,portMAX_DELAY);
    bool retry = request.active && !request.suspended && snapshot.status != MUSIC_UI_READY && snapshot.status != MUSIC_UI_LOADING;
    if (request.active && !request.suspended && snapshot.status == MUSIC_UI_READY) request.play = !request.play;
    xSemaphoreGive(lock);
    if (retry) music_service_open();
    else xTaskNotifyGive(task);
}
void music_service_suspend(bool suspended)
{
    if (!task) return;
    xSemaphoreTake(lock, portMAX_DELAY);
    request.suspended = suspended;
    if (suspended) request.play = false;
    xSemaphoreGive(lock);
    xTaskNotifyGive(task);
}
void music_service_step(int direction)
{
    if (!task || !direction) return;
    xSemaphoreTake(lock,portMAX_DELAY);
    if (request.active && snapshot.count) {
        request.index = direction > 0 ? (request.index+1)%snapshot.count : (request.index+snapshot.count-1)%snapshot.count;
        ++request.selection;
    }
    xSemaphoreGive(lock); xTaskNotifyGive(task);
}
void music_service_snapshot(music_snapshot_t *out)
{
    if (!lock) { *out = (music_snapshot_t){.status=MUSIC_UI_ERROR}; return; }
    xSemaphoreTake(lock,portMAX_DELAY); *out = snapshot; xSemaphoreGive(lock);
}
