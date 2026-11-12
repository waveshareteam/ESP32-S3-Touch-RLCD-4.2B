/* Offline generator: cc make_icons.c -lm -o make_icons; ./make_icons > weather_icons.c */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

enum { W=144,H=108,COUNT=8 };
static uint8_t image[H][W];
static void pixel(int x,int y,bool black) { if(x>=0&&x<W&&y>=0&&y<H) image[y][x]=black; }
static bool disk(double x,double y,double cx,double cy,double r) { return (x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r; }
static bool mask(double x,double y) { return disk(x,y,24,43,22)||disk(x,y,49,27,26)||disk(x,y,77,43,22)||(x>=24&&x<=77&&y>=41&&y<=65); }
static void paint_cloud(int ox,int oy,double scale,int stroke)
{
    for(int y=0;y<H;y++)for(int x=0;x<W;x++){
        double sx=(x-ox)/scale,sy=(y-oy)/scale;
        if(!mask(sx,sy))continue;
        bool interior=true;
        for(int dy=-stroke;dy<=stroke&&interior;dy++)for(int dx=-stroke;dx<=stroke;dx++)
            if(dx*dx+dy*dy<=stroke*stroke&&!mask(sx+dx/scale,sy+dy/scale)){interior=false;break;}
        pixel(x,y,!interior);
    }
}
static void line(int x0,int y0,int x1,int y1,int width)
{
    int steps=(int)fmax(abs(x1-x0),abs(y1-y0));
    for(int i=0;i<=steps;i++){
        int x=x0+(steps?(x1-x0)*i/steps:0),y=y0+(steps?(y1-y0)*i/steps:0);
        for(int dy=-width/2;dy<=width/2;dy++)for(int dx=-width/2;dx<=width/2;dx++)pixel(x+dx,y+dy,true);
    }
}
static void sun(void)
{
    for(int y=0;y<H;y++)for(int x=0;x<W;x++)if(disk(x,y,72,54,24)&&!disk(x,y,72,54,20))pixel(x,y,true);
    for(int i=0;i<8;i++){
        double angle=i*3.141592653589793/4;
        line(72+(int)round(33*cos(angle)),54+(int)round(33*sin(angle)),
             72+(int)round(45*cos(angle)),54+(int)round(45*sin(angle)),3);
    }
}
static void bolt(void)
{
    const int p[][2]={{77,67},{61,88},{73,88},{67,106},{95,80},{80,80},{89,67}};
    for(int y=65;y<H;y++)for(int x=55;x<101;x++){
        bool inside=false;
        for(int i=0,j=6;i<7;j=i++)if((p[i][1]>y)!=(p[j][1]>y)&&x<(double)(p[j][0]-p[i][0])*(y-p[i][1])/(p[j][1]-p[i][1])+p[i][0])inside=!inside;
        if(inside)pixel(x,y,true);
    }
}
static void emit(const char *name,int width,int height,const uint8_t *bits)
{
    printf("static const LV_ATTRIBUTE_MEM_ALIGN uint8_t %s_map[] = {\n    0,0,0,255,255,255,255,255,\n",name);
    for(int y=0;y<height;y++){
        printf("    ");
        for(int byte=0;byte<width/8;byte++){
            uint8_t value=0;
            for(int bit=0;bit<8;bit++)if(!bits[y*width+byte*8+bit])value|=1u<<(7-bit);
            printf("0x%02x,",value);
        }
        printf("\n");
    }
    printf("};\nconst lv_image_dsc_t %s = {\n    .header.magic=LV_IMAGE_HEADER_MAGIC, .header.cf=LV_COLOR_FORMAT_I1,\n    .header.w=%d, .header.h=%d, .header.stride=%d,\n    .data_size=sizeof(%s_map), .data=%s_map,\n};\n\n",name,width,height,width/8,name,name);
}
int main(void)
{
    puts("/* Generated pure black/white I1 icons; regenerate with assets/make_icons.c. */\n#include \"weather_icons.h\"\n");
    for(int type=0;type<COUNT;type++){
        memset(image,0,sizeof(image));
        switch(type){
        case 0:
            for(int y=22;y<58;y++)for(int x=52;x<94;x++)if(disk(x,y,72,42,20)&&!disk(x,y,72,42,16)&&!(x<72&&y>41))pixel(x,y,true);
            line(78,60,70,69,3);line(70,69,70,78,3);line(70,89,70,91,4);break;
        case 1:sun();break;
        case 2:paint_cloud(41,3,.78,3);paint_cloud(4,32,1.05,3);break;
        case 3:paint_cloud(10,12,1.2,4);break;
        case 4:paint_cloud(16,4,1.12,3);for(int x=44;x<120;x+=30)line(x,84,x-7,99,3);break;
        case 5:paint_cloud(16,0,1.12,3);bolt();break;
        case 6:paint_cloud(16,0,1.1,3);for(int x=39;x<130;x+=32){line(x-6,90,x+6,90,1);line(x-4,85,x+4,95,1);line(x-4,95,x+4,85,1);}break;
        case 7:paint_cloud(16,0,1.05,3);line(23,78,119,78,3);line(35,91,128,91,3);line(24,104,111,104,3);break;
        }
        char name[32];snprintf(name,sizeof(name),"weather_icon_%d",type);emit(name,W,H,&image[0][0]);
    }
    puts("const lv_image_dsc_t *weather_icon_get(weather_condition_t condition)\n{\n    static const lv_image_dsc_t *icons[]={&weather_icon_0,&weather_icon_1,&weather_icon_2,&weather_icon_3,&weather_icon_4,&weather_icon_5,&weather_icon_6,&weather_icon_7};\n    return icons[(unsigned)condition<WEATHER_ICON_COUNT?condition:WEATHER_UNKNOWN];\n}");
    return 0;
}
