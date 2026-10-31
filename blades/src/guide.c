#include "guide.h"
#include "guide_art.h"
#include "asset_codec.h"
#include "draw.h"
#include "font.h"
#include "ui_audio.h"
#include <stdio.h>
#include <string.h>

enum { MAIN, SIGN_IN, INFO, DASHBOARD };
static uint32_t *surfaces[GUIDE_ART_COUNT];
static uint32_t panel_cache[586*720];
static int panel_dirty=1;
static int page, focus, music_focus, parent, selection, target, dirty;
static int sound, pending_action, action, hour=-1, minute=-1;
static float progress, shade, repeat_wait;
static BladesHeld previous;
static char user[32];
static const char *notice_title,*notice_message;
static const float slide_seconds=.20f; /* Provisional until Guide footage is measured. */

static void changed(int cue) { dirty=1;panel_dirty=1;if(cue)sound=cue; }
static void open_page(int next,int from)
{
    int was_open=guide_active();
    int was_right=page==DASHBOARD||(page==INFO&&parent<0);
    page=next;parent=from;target=1;
    int now_right=page==DASHBOARD||(page==INFO&&parent<0);
    if(!was_open||was_right!=now_right)progress=0;
    changed(was_open?UI_SOUND_SELECT:UI_SOUND_GUIDE_OPEN);
}
static void close_guide(void) {target=0;changed(UI_SOUND_GUIDE_CLOSE);}
static void info(const char *title,const char *message,int from)
{
    notice_title=title;notice_message=message;open_page(INFO,from);
}
int guide_init(void)
{
    for(int i=0;i<GUIDE_ART_COUNT;i++) {
        if(!surfaces[i])surfaces[i]=asset_decode(&guide_art[i]);
        if(!surfaces[i])return 0;
    }
    page=MAIN;focus=0;music_focus=0;parent=MAIN;selection=1;target=0;
    panel_dirty=1;dirty=0;sound=0;pending_action=0;action=0;progress=0;shade=0;repeat_wait=0;
    memset(&previous,0,sizeof previous);user[0]=0;hour=-1;minute=-1;
    return 1;
}
int guide_active(void) {return target||progress>0||shade>0;}
void guide_dismiss(void)
{
    target=0;progress=0;shade=0;pending_action=0;action=0;sound=0;
    memset(&previous,0,sizeof previous);repeat_wait=0;dirty=1;
}
void guide_notice(const char *title,const char *message) {info(title,message,-1);}
int guide_take_sound(void) {int cue=sound;sound=0;return cue;}
int guide_take_action(void) {int result=action;action=0;return result;}
void guide_set_user(const char *gamertag)
{
    char next[sizeof user]={0};
    if(gamertag)snprintf(next,sizeof next,"%s",gamertag);
    if(strcmp(user,next)) {memcpy(user,next,sizeof user);dirty=1;panel_dirty=1;}
}
void guide_set_clock(int h,int m)
{
    if(h<0||h>23||m<0||m>59)return;
    if(hour!=h||minute!=m) {hour=h;minute=m;dirty=1;panel_dirty=1;}
}
static void go_back(void)
{
    if(page==MAIN||(page==INFO&&parent<0))close_guide();
    else if(page==INFO) {int back=parent;open_page(back,MAIN);changed(UI_SOUND_BACK);}
    else {open_page(MAIN,MAIN);changed(UI_SOUND_BACK);}
}
static void select_item(void)
{
    if(page==INFO) {go_back();return;}
    if(page==DASHBOARD) {
        if(selection==0) {pending_action=GUIDE_ACTION_DASHBOARD;close_guide();}
        else go_back();
        return;
    }
    if(page==SIGN_IN) {
        info("Sign In",user[0]?"A profile is signed in. Profile switching is not available here.":"Profile sign-in is not available in this launcher yet.",SIGN_IN);
        return;
    }
    switch(focus) {
    case 0:open_page(SIGN_IN,MAIN);break;
    case 1:info("Create New Profile","Profile creation is not available in this launcher yet.",MAIN);break;
    case 2:info("Recover Gamertag","Xbox LIVE is unavailable. Connect to Xbox LIVE to recover a gamertag.",MAIN);break;
    case 3:info(user[0]?"Personal Settings":"Profile Required",user[0]?"Personal settings are not available in this launcher yet.":"You must be signed in to change personal settings.",MAIN);break;
    case 4:info("Music","No music is selected.",MAIN);break;
    case 5:info("Select Music","No music source is connected.",MAIN);break;
    case 6:selection=1;open_page(DASHBOARD,MAIN);break;
    }
}
static void on_press(int up,int down,int left,int right,int a,int b,int x,int y)
{
    if(b) {go_back();return;}
    if(x&&(page==MAIN||page==SIGN_IN)) {
        if(page==MAIN)open_page(SIGN_IN,MAIN);
        else info("Recover Gamertag","Xbox LIVE is unavailable. Connect to Xbox LIVE to recover a gamertag.",SIGN_IN);
        return;
    }
    if(y&&(page==MAIN||page==SIGN_IN)) {
        if(page==MAIN) {selection=1;open_page(DASHBOARD,MAIN);}
        else info("Create New Profile","Profile creation is not available in this launcher yet.",SIGN_IN);
        return;
    }
    if(page==MAIN) {
        int before=focus;
        if(up&&focus>0)focus--;
        if(down&&focus<6)focus++;
        if(before!=focus)changed(UI_SOUND_MOVE);
        if(focus==4&&(left||right)) {
            int old=music_focus;music_focus+=right?1:-1;
            if(music_focus<0)music_focus=0;
            if(music_focus>5)music_focus=5;
            if(old!=music_focus)changed(UI_SOUND_MOVE);
        }
    } else if(page==DASHBOARD) {
        int before=selection;
        if(up)selection=0;
        if(down)selection=1;
        if(before!=selection)changed(UI_SOUND_MOVE);
    }
    if(a) {changed(UI_SOUND_SELECT);select_item();}
}
int guide_frame(const BladesHeld *held,float dt)
{
    int toggle=held->guide&&!previous.guide;
    int up=held->up&&!previous.up,down=held->down&&!previous.down;
    int left=held->left&&!previous.left,right=held->right&&!previous.right;
    int a=held->a&&!previous.a,b=held->b&&!previous.b;
    int x=held->x&&!previous.x,y=held->y&&!previous.y;
    if(toggle) {
        if(target)close_guide();
        else {page=MAIN;focus=0;pending_action=0;open_page(MAIN,MAIN);}
        repeat_wait=.38f;
    } else if(target) {
        if(up||down||left||right||a||b||x||y) {
            on_press(up,down,left,right,a,b,x,y);repeat_wait=.38f;
        } else if(held->up||held->down||held->left||held->right) {
            repeat_wait-=dt;
            if(repeat_wait<=0) {on_press(held->up,held->down,held->left,held->right,0,0,0,0);repeat_wait=.11f;}
        }
    }
    float before=progress,old_shade=shade;
    float step=dt/slide_seconds;
    progress+=target?step:-step;
    shade+=target?step:-step;
    if(progress<0)progress=0;
    if(progress>1)progress=1;
    if(shade<0)shade=0;
    if(shade>1)shade=1;
    if(before!=progress||old_shade!=shade)dirty=1;
    if(!target&&progress==0&&shade==0&&pending_action) {
        action=pending_action;pending_action=0;dirty=1;
    }
    previous=*held;
    int repaint=dirty;dirty=0;return repaint;
}
static void draw_text(int x,int y,int size,const char *s) {font_draw(x,y,size,0xFF171B13,s);}
static void white_text(int x,int y,int size,const char *s) {font_draw(x,y,size,0xFFF2F2EA,s);}
static void row(int x,int y,int w,int h)
{
    for(int line=0;line<h;line++) {
        int alpha=54-34*line/(h>1?h-1:1);
        draw_fill(x,y+line,w,1,((uint32_t)alpha<<24)|0xFFFFFF);
    }
    draw_fill(x,y,w,2,0x88BED69E);draw_fill(x,y+h-2,w,2,0x886A7B58);
}
static void panel(int id,int dx)
{
    int w=guide_art[id].w;
    for(int y=0;y<720;y++) {
        int x0=id==GUIDE_ART_RIGHT?guide_right_edge[y]:0;
        int x1=id==GUIDE_ART_RIGHT?w:guide_left_edge[y];
        draw_argb(dx+x0,y,x1-x0,1,surfaces[id]+y*w+x0);
    }
}
static void panel_footer(int dx,int right)
{
    /* Colored button orbs and Back/Select are original reference pixels. */
    if(!right) {
        for(int y=570;y<646;y++)draw_argb(dx+370,y,110,1,surfaces[GUIDE_ART_MAIN]+y*570+370);
    }
}
static void paint_panel(int id,int right,int dx)
{
    panel(id,dx);
    if(!right) {
        char clock[20]="--:--";
        if(hour>=0)snprintf(clock,sizeof clock,"%d:%02d %s",hour%12?hour%12:12,minute,hour>=12?"PM":"AM");
        white_text(dx+476-font_width(30,clock),56,30,clock);
    }
    if(page==MAIN) {
        /* Neutral avatar until the actual user's gamerpic can be retrieved. */
        draw_fill(dx+120,119,56,61,0xFFABB4A1);
        draw_icon(dx+124,123,48,0xFFE1E6D9,ICON_FACE);
        draw_text(dx+186,114,31,user[0]?user:"Sign In");
        draw_text(dx+186,146,30,user[0]?"Signed In":"Offline");
        const int ys[]={107,208,263,327,383,428,477};
        const int hs[]={85,55,64,40,40,33,77};
        if(focus==4)row(dx+108+music_focus*60,385,55,38);
        else row(dx+104,ys[focus],366,hs[focus]);
    } else if(page==SIGN_IN) {
        white_text(dx+102,53,36,"Sign In");
        if(user[0]) {
            row(dx+104,224,364,66);draw_text(dx+120,230,32,user);
            draw_text(dx+120,268,27,"Signed In");
        } else font_wrap(dx+104,220,370,30,30,0xFF171B13,"Profile sign-in is not available in this launcher yet.");
        /* Original colored X/Y prompts, with native labels for their actions. */
        for(int y=576;y<642;y++)draw_argb(dx+96,y,42,1,surfaces[GUIDE_ART_MAIN]+y*570+96);
        white_text(dx+140,581,29,"Create New Profile");white_text(dx+124,608,29,"Recover Gamertag");
        panel_footer(dx,0);
    } else if(page==INFO) {
        int tx=dx+(right?98:102);
        if(right)draw_argb(dx+96,58,28,28,surfaces[GUIDE_ART_ERROR]);
        white_text(tx+(right?34:0),53,36,notice_title);
        font_wrap(tx,112,right?385:365,32,31,0xFF171B13,notice_message);
        row(tx,496,right?382:365,44);draw_text(tx+8,499,31,"OK");
        panel_footer(dx,right);
    } else {
        int tx=dx+98;
        draw_argb(dx+96,58,28,28,surfaces[GUIDE_ART_WARNING]);
        white_text(tx+34,53,36,"Xbox Dashboard");
        font_wrap(tx,112,384,32,31,0xFF171B13,"This will exit your current session. If you are playing a game, you will lose any unsaved progress. Are you sure you want to exit?");
        row(tx,selection?496:454,382,44);
        draw_text(tx+8,457,31,"Yes");draw_text(tx+8,499,31,"No");
    }
}

void guide_paint(void)
{
    if(!guide_active())return;
    float u=progress*progress*(3-2*progress);
    draw_dim((int)(shade*190));
    int right=page==DASHBOARD||(page==INFO&&parent<0);
    int id=right?GUIDE_ART_RIGHT:page==MAIN?GUIDE_ART_MAIN:GUIDE_ART_EMPTY;
    int w=guide_art[id].w;
    if(panel_dirty) {
        draw_target(panel_cache,w,720);
        paint_panel(id,right,0);
        draw_target(NULL,0,0);
        panel_dirty=0;
    }
    int dx=right?1280-w+(int)(w*(1-u)):-(int)(w*(1-u));
    for(int y=0;y<720;y++) {
        int x0=right?guide_right_edge[y]:0;
        int x1=right?w:guide_left_edge[y];
        draw_argb(dx+x0,y,x1-x0,1,panel_cache+y*w+x0);
    }
}
