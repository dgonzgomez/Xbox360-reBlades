#include "blades.h"
#include "draw.h"
#include "font.h"
#include "reference_assets.h"
#include "ui_audio.h"
#include "guide.h"
#include "library.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Canonical reference: Jonah360's five-blade widescreen walkthrough.
 * Geometry is measured at 1280x720. Static chrome and branding are component
 * textures; menus, focus, page hierarchy and accordion movement are native.
 * Console services and the reference video's sample library are not connected. */
enum { MARKET, LIVE, GAMES, MEDIA, SYSTEM, BLADE_COUNT };
enum { HOME, LIBRARY, MY_GAMES, CONSOLE, DISPLAY, AUDIO, MEDIA_CENTER };
static const int base[] = { REF_MARKET_BASE, REF_LIVE_BASE, REF_GAMES_BASE, REF_MEDIA_BASE, REF_SYSTEM_BASE };
static const int shell[] = { REF_MARKET_SHELL, REF_LIVE_SHELL, REF_GAMES_SHELL, REF_MEDIA_SHELL, REF_SYSTEM_SHELL };
static const char *market_items[] = { "Sign In", "Join Xbox LIVE", "Recover Gamertag" };
static const char *live_items[] = { "Join Xbox LIVE", "Recover Gamertag\nfrom Xbox LIVE" };
static const char *games_items[] = { "Create Gamer Profile", "Games Library" };
static const char *media_items[] = { "Music", "Pictures", "Videos", "Media Center" };
static const char *system_items[] = { "Console Settings", "Family Settings", "Memory", "Network Settings", "Computers", "Xbox LIVE Vision", "Initial Setup" };
static const char *console_items[] = { "Display", "Audio", "Themes", "Language", "Clock", "Locale", "Startup", "Shutdown", "Screen Saver" };
static const char *game_names[] = { "3D Ultra Minigolf", "BAYONETTA First Climax Demo", "Bejeweled Blitz LIVE", "Bomberman LIVE", "Boom Boom Rocket", "Burnout Revenge (Demo)", "Cloning Clyde", "Crazy Taxi" };
static const char *tab_names[] = { "All Games", "Arcade", "Demos", "Recently Downloaded" };

static int blade, page, focus[BLADE_COUNT], subfocus, game_i, tab;
static int pending_a, assets_ready;
static int system_ui, await_release;
static BladesHeld previous;
static float repeat_wait, slide_time, focus_vis;
static int from_blade, direction, requested_blade;
static uint32_t incoming[BLADES_WIDTH * BLADES_HEIGHT];
static uint32_t guide_backdrop[BLADES_WIDTH * BLADES_HEIGHT];
static int guide_backdrop_valid;
static int sound_event;
static BladesGame installed[BLADES_MAX_GAMES];
static int installed_count,console_library,library_unavailable;
static char current_user[32];
static void paint(void);
void blades_set_library(const BladesGame *games,int count)
{
    console_library=1;library_unavailable=count<0;
    if(count<0)count=0;
    if(count>BLADES_MAX_GAMES)count=BLADES_MAX_GAMES;
    installed_count=count;
    if(games&&count)memcpy(installed,games,(size_t)count*sizeof *games);
    else installed_count=0;
    game_i=0;focus_vis=0;guide_backdrop_valid=0;
    if(assets_ready)paint();
}


int blades_take_sound(void) { int event=sound_event;sound_event=0;return event; }
static const uint32_t ink = 0xFF101B09;

static void text(int x,int y,int size,const char *s) { font_draw(x,y,size,ink,s); }
static void ellipsized(int x,int y,int w,int size,const char *s)
{
    char shown[128];
    if(font_width(size,s)<=w) {text(x,y,size,s);return;}
    size_t n=strlen(s);
    if(n>sizeof shown-4)n=sizeof shown-4;
    memcpy(shown,s,n);shown[n]=0;
    while(n>0) {
        memcpy(shown+n,"...",4);
        if(font_width(size,shown)<=w)break;
        shown[--n]=0;
    }
    text(x,y,size,shown);
}
static void paragraph(int x,int y,int w,int size,int lh,const char *s) { font_wrap(x,y,w,size,lh,ink,s); }
static void bullets(int x,int y,int size,int lh,const char *const *lines,int count)
{
    for(int i=0;i<count;i++) {
        draw_circle(x+5,y+i*lh+size/2,3,ink);
        text(x+22,y+i*lh,size,lines[i]);
    }
}
static void shadow_text(int x,int y,int size,const char *s)
{
    font_draw(x+1,y+2,size,0xFF3B5141,s);
    font_draw(x,y,size,0xFFF0F0E7,s);
}
static void orb(int x,int y,uint32_t color,const char *s)
{
    draw_circle(x+15,y+17,16,0x70000000);
    draw_circle(x+15,y+15,14,color);
    draw_circle(x+11,y+9,6,0x77FFFFFF);
    font_draw(x+7,y-1,28,0xFF18320C,s);
}
static void footer(int back,int select)
{
    if (back) { shadow_text(1012,600,38,"Back"); orb(1080,603,0xFFE44632,"B"); }
    if (select) { shadow_text(1016,633,38,"Select"); orb(1101,636,0xFF6BCC48,"A"); }
}
static void selected_row(int x,int y,int w,int h,int selected)
{
    if (selected) {
        /* Settled focus in the widescreen footage brightens toward its
         * curved right end. Keep the plate translucent over the live glass. */
        for(int col=0;col<w;col++) {
            int d=w-1-col;
            int inset=d<8?8-(int)sqrtf((float)(64-(8-d)*(8-d))):0;
            int alpha=12+100*col/(w-1);
            draw_fill(x+col,y+inset,1,h-2*inset,((uint32_t)alpha<<24)|0xFFFFFF);
        }
        draw_fill(x,y,w-8,1,0x667CAA6B);
        draw_fill(x,y+h-2,w-8,2,0x88405934);
    } else draw_fill(x,y+h-2,w,2,0x55465839);
}
static void unavailable(const char *title,const char *message)
{
    guide_notice(title,message);
}
static int home_count(int b)
{
    const int counts[]={3,5,4,6,7}; /* Sign-in cards are selectable on Live/Games/Media. */
    return counts[b];
}

static void profile_card(int x)
{
    uint32_t metal=blade==LIVE?0xFF83ADC3:blade==MEDIA?0xFFA0B2BB:0xFF74AD61;
    draw_round(x,94,396,137,12,0xFF44644B);
    draw_round(x+2,96,392,133,10,metal);
    text(x+10,99,30,current_user[0]?"Profile":"Sign In");
    draw_icon(x+14,139,72,0xFFD7E0D2,ICON_FACE);
    ellipsized(x+100,146,280,34,current_user[0]?current_user:"Offline");
}
static void paint_home(void)
{
    int f=focus[blade];
    reference_blit(base[blade],0,0);
    if(blade==LIVE||blade==GAMES||blade==MEDIA)profile_card(208+(blade-LIVE)*46);
    if (blade==MARKET) {
        for(int i=0;i<3;i++) {
            int y=434+i*50;
            selected_row(560,y,398,50,i==f);
            reference_blit(REF_MARKET_ICON0+i,568,y+5);
            text(622,y+4,38,market_items[i]);
        }
    } else if(blade==LIVE) {
        if(f==0) draw_round(208,94,396,137,12,0x20FFFFFF);
        for(int i=0;i<2;i++) {
            int y=244+i*77;
            selected_row(208,y,396,77,f==i+1);
            reference_blit(REF_LIVE_ICON0+i,230,y+4);
            paragraph(312,y+6,290,38,31,live_items[i]);
        }
        if(f==3) draw_round(208,404,396,102,12,0x22FFFFFF);
        if(f==4) draw_round(206,514,784,74,36,0x22FFFFFF);
        paragraph(622,250,386,38,33,"Xbox LIVE\nGames. Tournaments. Entertainment. All the rewards. Endless possibilities. What are you waiting for?");
    } else if(blade==GAMES) {
        if(f==0) draw_round(254,94,396,137,12,0x20FFFFFF);
        for(int i=0;i<2;i++) {
            int y=244+i*50;
            selected_row(260,y,390,50,f==i+1);
            reference_blit(REF_GAMES_ICON0+i,272,y+6);
            text(322,y+7,37,games_items[i]);
        }
        if(f==3) draw_round(250,514,784,74,36,0x22FFFFFF);
        if(f==2) {
            static const char *lines[]={"Demos","Xbox Originals","XNA Creators Club","Xbox LIVE Arcade"};
            text(668,249,37,"Play games on your console.");
            bullets(668,282,37,33,lines,4);
        }
        else if(f==1) paragraph(666,246,388,38,33,"Every gamer needs a profile.\nSet up yours and play offline right away.");
        else paragraph(666,246,388,38,33,"Xbox LIVE\nGames. Tournaments. Entertainment. All the rewards. Endless possibilities. What are you waiting for?");
    } else if(blade==MEDIA) {
        static const char *descriptions[]={
            "Xbox LIVE\nGames. Tournaments. Entertainment. All the rewards. Endless possibilities. What are you waiting for?",
            "Listen to music stored on your Xbox 360 console.",
            "View pictures from a camera, portable device, or computer.",
            "Watch videos stored on your Xbox 360 console.",
            "Connect or set up Media Center Extender.\n- Live & recorded TV\n- On-demand movies\n- Music and photos"};
        if(f==0) draw_round(300,94,396,137,12,0x20FFFFFF);
        for(int i=0;i<4;i++) {
            int y=244+i*50;
            selected_row(304,y,390,50,f==i+1);
            reference_blit(REF_MEDIA_ICON0+i,310,y+4);
            text(366,y+4,38,media_items[i]);
        }
        if(f==5) draw_round(294,514,784,74,36,0x22FFFFFF);
        paragraph(708,246,386,38,33,descriptions[f<5?f:0]);
    } else {
        for(int i=0;i<7;i++) {
            int y=104+i*49;
            selected_row(340,y,404,49,i==f);
            reference_blit(REF_SYSTEM_ICON0+i,346,y+4);
            text(398,y+4,38,system_items[i]);
        }
        if(f==0) paragraph(764,104,380,38,33,"Edit your Xbox 360 system settings, including:\n\n- Display\n- Audio\n- Language\n- Remote control\n- and more");
        else text(764,104,38,system_items[f]);
    }
}

static void paint_library(void)
{
    reference_blit(REF_GAMES_LIBRARY_BASE,0,0);
    reference_blit(REF_LIBRARY_BANNER,164,94);
    for(int i=0;i<5;i++) {
        int y=178+i*82;
        reference_blit(REF_LIBRARY_ROW0+i,164,y);
        if(i==subfocus) {
            draw_round(164,y,484,76,12,0x10FFFFFF);
            draw_fill(174,y+1,464,1,0xAADEF5D2);
            draw_fill(174,y+74,464,1,0x996EAD55);
        }
        if(i==0) {
            /* Replace the captured demo count with the live reference-list count. */
            for(int x=598;x<642;x++)
                reference_rect(REF_LIBRARY_ROW0,416,24,1,48,x,y+24);
            char total[16];snprintf(total,sizeof total,"%d",console_library?installed_count:8);
            text(598,y+30,37,total);
        }
    }
    static const char *descs[]={"My Games\nYou have 8 games in this reference library. Select this option to browse them.","Last Played Game\nYou haven't played a game recently.","Friends Playing Now\nYou have no friends online playing games.","Download Games\nChoose from a variety of games including Xbox LIVE Arcade, Xbox Originals, demos, and more.","Auto Downloads\nAutomatically download free trial versions of new Xbox LIVE Arcade games as they become available."};
    if(console_library&&subfocus==0) {
        char desc[200];snprintf(desc,sizeof desc,"My Games\n%d installed GOD packages. Select to browse your hard drive.",installed_count);
        paragraph(664,100,425,38,34,library_unavailable?"My Games\nUnable to read the HDD Content folder. See blades.log.":desc);
    } else paragraph(664,100,425,38,34,descs[subfocus]);
}
static int game_count(void) { return console_library?(tab==0?installed_count:0):(tab==0?8:tab==1?6:tab==2?2:0); }
static int game_at(int index)
{
    if(console_library)return index;
    static const int arcade[]={0,2,3,4,6,7};
    static const int demos[]={1,5};
    return tab==1?arcade[index]:tab==2?demos[index]:index;
}
static void paint_games(void)
{
    reference_blit(REF_MY_GAMES_BASE,0,0);
    const int tx[]={176,376,532,688};
    for(int i=0;i<4;i++) font_draw(tx[i],100,39,i==tab?0xFFF0F2E3:ink,tab_names[i]);
    int y=160,n=game_count();
    int start=game_i>7?game_i-7:0;
    for(int i=start;i<n&&i<start+8;i++) {
        int g=game_at(i);
        float emphasis=1.0f-fabsf(i-focus_vis);
        if(emphasis<0) emphasis=0;
        int h=46+(int)(34*emphasis);
        selected_row(164,y,476,h,i==game_i);
        int size=36+(int)(28*emphasis);
        if(console_library)draw_icon(178,y+6,size,0xFFCFDFC0,ICON_CONTROLLER);
        else reference_blit_scaled(REF_GAME_ICON0+g,178,y+6,size,size);
        draw_clip(222+(int)(34*emphasis),y,410-(int)(34*emphasis),h);
        ellipsized(222+(int)(34*emphasis),y+3,410-(int)(34*emphasis),38,console_library?installed[g].name:game_names[g]);
        draw_clip_none();
        y+=h;
    }
    draw_round(650,156,462,422,12,0xAA244719);
    draw_round(653,159,456,416,10,0xAA69BA50);
    if(n) {
        int g=game_at(game_i);
        text(664,164,38,console_library?"Xbox 360 Game":g==1||g==5?"Game Demo":"Arcade Game");
        if(console_library)draw_icon(680,220,68,0xFFCFDFC0,ICON_CONTROLLER);
        else reference_blit_scaled(REF_GAME_ICON0+g,680,220,68,68);
        paragraph(768,212,325,38,33,console_library?installed[g].name:game_names[g]);
        draw_fill(654,302,454,40,0x224C772B);
        text(664,302,38,console_library?"Installed on Hard Drive":g==1||g==5?"Description":"Achievements");
        const char *desc=g==1?"Get ready for your senses to be pushed to the limit as Bayonetta faces off against angelic enemies and giant foes.":"Unlock the full game to earn achievements and track your high score.";
        if(console_library) {char info[160];snprintf(info,sizeof info,"Title ID: %08X\nGames on Demand\nLaunch from Aurora while launcher integration is being completed.",(unsigned)installed[g].title_id);paragraph(664,344,424,32,30,info);}
        else paragraph(664,344,424,38,33,desc);
        char count[32];snprintf(count,sizeof count,"%d of %d",game_i+1,n);text(470,554,32,count);
    } else paragraph(178,172,800,34,34,console_library?(library_unavailable?"Unable to read HDD Content. See blades.log.":tab?"No GOD games in this category.":"No GOD packages found in HDD Content."):"No games");
}
static void paint_console(void)
{
    reference_blit(REF_CONSOLE_SETTINGS_BASE,0,0);
    if(page==CONSOLE) {
        for(int i=0;i<9;i++) {
            int y=104+i*49;
            selected_row(168,y,494,49,i==subfocus);
            text(178,y+4,38,console_items[i]);
        }
        if(subfocus==0) paragraph(686,104,425,38,33,"Current Setting\n720p\nWidescreen\nStandard\n\nChange your display output settings. Display settings include screen size, output mode, and HDTV.");
        else if(subfocus==1) paragraph(686,104,425,38,33,"Current Setting\n\nChange your audio output and sound effects settings.");
        else text(686,104,38,console_items[subfocus]);
    } else if(page==DISPLAY) {
        draw_fill(160,0,960,86,0xFFA37CCE);shadow_text(178,38,44,"Display");
        const char *items[]={"HDTV Settings","Screen Format","Reference Levels"};
        for(int i=0;i<3;i++) { selected_row(168,104+i*49,494,49,i==subfocus);text(178,108+i*49,38,items[i]); }
        paragraph(686,104,425,38,33,"Current Setting\n720p\nWidescreen\nStandard");
    } else {
        draw_fill(160,0,960,86,0xFFA37CCE);shadow_text(178,38,44,"Audio");
        selected_row(168,104,494,49,subfocus==0);text(178,108,38,"Digital Output");
        selected_row(168,153,494,49,subfocus==1);text(178,157,38,"Sound Effects");
        paragraph(686,104,425,38,33,"Current Setting\n\nSelect Digital Output to change digital audio output settings. Select Sound Effects to mute sound effects in the Xbox Dashboard and Xbox Guide.");
    }
}
static void paint_media_center(void)
{
    reference_blit(REF_MEDIA_BASE,0,0);
    draw_gradient(0,0,1280,720,0xFF377CB0,0xFF348DCC);
    draw_fill(0,0,1280,86,0x33000000);shadow_text(178,38,44,"Windows Media Center");
    paragraph(182,110,950,36,33,"A PC running Windows Media Center is the all-in-one entertainment and computing solution for your home. For more info, go to:");
    text(452,256,44,"www.microsoft.com/extender");
    paragraph(182,382,950,36,33,"To connect your Xbox 360 console to a PC powered by Windows Media Center on your home network, select Windows Media Center now.");
    selected_row(394,514,516,52,1);text(538,522,32,"Windows Media Center");
    draw_fill(0,598,1280,122,0x22000000);footer(1,1);
}
static void paint_scene(void)
{
    if(page==HOME) paint_home();
    else if(page==LIBRARY) paint_library();
    else if(page==MY_GAMES) paint_games();
    else if(page==MEDIA_CENTER) paint_media_center();
    else paint_console();
}

/* Approximate fold key positions measured from the 60 fps Live->Games passage
 * at 24.6167..24.7833s. This is a measured starting curve, not a fidelity pass. */
static float fold_progress(float seconds)
{
    static const float keys[]={0,.163f,.284f,.455f,.569f,.646f,.753f,.826f,.902f,.975f,1};
    float f=seconds*60.0f;int i=(int)f;
    if(i>=10) return 1;
    if(i<0) return 0;
    return keys[i]+(keys[i+1]-keys[i])*(f-i);
}
static void paint_dashboard(void)
{
    if(slide_time>=.266667f) { paint_scene();return; }
    float u=fold_progress(slide_time);
    if(u>=1) {
        draw_target(incoming,BLADES_WIDTH,BLADES_HEIGHT);
        paint_scene();
        draw_target(0,0,0);
        reference_blit(shell[blade],0,0);
        int alpha=(int)((slide_time-.166667f)*2550);
        if(alpha<0)alpha=0;
        if(alpha>255)alpha=255;
        for(int y=0;y<BLADES_HEIGHT;y++) for(int x=0;x<BLADES_WIDTH;x++) {
            uint32_t p=incoming[y*BLADES_WIDTH+x];
            draw_blend(x,y,(p>>16)&255,(p>>8)&255,p&255,alpha);
        }
        return;
    }
    reference_blit(shell[from_blade],0,0);
    if(direction>0) {
        int dest_left=132+blade*46;
        int edge=(int)(1069+(dest_left+46-1069)*u);
        reference_rect(shell[blade],edge,0,1280-edge,720,edge,0);
        reference_rect(shell[blade],dest_left,0,46,720,edge-46,0);
    } else {
        int dest_right=996+blade*46;
        int edge=(int)(180+(dest_right-180)*u);
        reference_rect(shell[blade],0,0,edge,720,0,0);
        reference_rect(shell[blade],dest_right,0,46,720,edge,0);
    }
}
static void paint(void)
{
    if(guide_active()&&!system_ui) {
        if(!guide_backdrop_valid) {
            paint_dashboard();
            memcpy(guide_backdrop,blades_pixels(),sizeof guide_backdrop);
            guide_backdrop_valid=1;
        } else draw_argb(0,0,BLADES_WIDTH,BLADES_HEIGHT,guide_backdrop);
    } else {guide_backdrop_valid=0;paint_dashboard();}
    if(!system_ui)guide_paint();
}
void blades_set_user(const char *gamertag) {
    guide_set_user(gamertag);
    char next[sizeof current_user]={0};if(gamertag)snprintf(next,sizeof next,"%s",gamertag);
    if(strcmp(next,current_user)) {memcpy(current_user,next,sizeof next);if(assets_ready&&!guide_active())paint();}
}
void blades_set_clock(int hour,int minute) {guide_set_clock(hour,minute);}
void blades_system_ui_state(int open)
{
    if(open&&!system_ui) {
        guide_backdrop_valid=0;
        guide_dismiss();pending_a=0;requested_blade=blade;repeat_wait=.38f;
        memset(&previous,0,sizeof previous);
        if(assets_ready)paint_dashboard();
    }
    if(!open&&system_ui)await_release=1;
    system_ui=!!open;
}
static void activate(void)
{
    if(page==HOME) {
        int f=focus[blade];
        if(blade==GAMES && f==2) { page=LIBRARY;subfocus=0; }
        else if(blade==SYSTEM && f==0) { page=CONSOLE;subfocus=0; }
        else if(blade==MEDIA && f==4) page=MEDIA_CENTER;
        else unavailable("Xbox Dashboard","This service is not available in this standalone launcher.");
    } else if(page==LIBRARY && subfocus==0) {page=MY_GAMES;game_i=0;focus_vis=0;tab=0;}
    else if(page==CONSOLE && subfocus==0) {page=DISPLAY;subfocus=0;}
    else if(page==CONSOLE && subfocus==1) {page=AUDIO;subfocus=0;}
    else if(page==MY_GAMES) {
        if(game_count())unavailable("Game Launch",console_library?"Launch this GOD game from Aurora. Package discovery is connected; package launching is not available here yet.":"This reference game is not connected to an installed executable.");
    }
    else unavailable("Xbox Dashboard","This service is not available in this standalone launcher.");
}
static void back(void)
{
    pending_a=0;
    if(page==MY_GAMES) {page=LIBRARY;subfocus=0;}
    else if(page==DISPLAY || page==AUDIO) {subfocus=page==AUDIO?1:0;page=CONSOLE;}
    else {page=HOME;}
}
static void on_press(int up,int down,int left,int right,int a,int b)
{
    if(b) {back();sound_event=UI_SOUND_BACK;return;}
    if(page==HOME && (left||right)) {
        int next=requested_blade+(right?1:-1);
        if(next<0 || next>=BLADE_COUNT) return;
        requested_blade=next;
        if(slide_time>=.266667f) {
            sound_event=UI_SOUND_BLADE;
            from_blade=blade;blade=next;direction=right?1:-1;slide_time=0;
        }
        pending_a=0;
        return;
    }
    if(page==MY_GAMES && (left||right)) {
        tab+=right?1:-1;if(tab<0)tab=0;if(tab>3)tab=3;game_i=0;focus_vis=0;
    }
    int *current=page==HOME?&focus[blade]:page==MY_GAMES?&game_i:&subfocus;
    int count=page==HOME?home_count(blade):page==LIBRARY?5:page==MY_GAMES?game_count():page==CONSOLE?9:page==DISPLAY?3:page==AUDIO?2:1;
    if(up && *current>0) {--*current;sound_event=UI_SOUND_MOVE;}
    if(down && *current+1<count) {++*current;sound_event=UI_SOUND_MOVE;}
    if(a) {sound_event=UI_SOUND_SELECT;if(slide_time<.266667f) pending_a=1;else activate();}
}
void blades_init(void)
{
    guide_backdrop_valid=0;console_library=0;installed_count=0;library_unavailable=0;current_user[0]=0;
    memset(focus,0,sizeof focus);memset(&previous,0,sizeof previous);
    blade=GAMES;focus[GAMES]=2;page=HOME;subfocus=0;game_i=0;tab=0;system_ui=0;await_release=0;
    sound_event=0;slide_time=1;repeat_wait=0;focus_vis=0;pending_a=0;from_blade=GAMES;requested_blade=GAMES;direction=1;
    draw_target(0,0,0);draw_clip_none();font_init();
    assets_ready=reference_assets_init()&&guide_init();
    if(assets_ready) paint();else {draw_fill(0,0,1280,720,0xFF102010);shadow_text(100,100,36,"Unable to load dashboard assets");}
}
void blades_frame(const BladesHeld *held,float dt)
{
    if(!assets_ready) return;
    if(dt<0)dt=0;
    if(dt>.1f)dt=.1f;
    if(system_ui)return;
    if(await_release) {
        if(!(held->up||held->down||held->left||held->right||held->a||held->b||held->x||held->y||held->guide)) {
            await_release=0;memset(&previous,0,sizeof previous);
            guide_frame(&(BladesHeld){0},0);
        }
        return;
    }
    int was_guide=guide_active();
    int guide_repaint=guide_frame(held,dt);
    int guide_sound=guide_take_sound();
    if(guide_sound)sound_event=guide_sound;
    if(was_guide||guide_active()) {
        previous=*held;repeat_wait=.38f;
        if(guide_take_action()==GUIDE_ACTION_DASHBOARD) {
            page=HOME;blade=GAMES;focus[GAMES]=2;slide_time=1;
            from_blade=GAMES;requested_blade=GAMES;pending_a=0;
        }
        if(!guide_active())await_release=1;
        if(guide_repaint)paint();
        return;
    }
    int up=held->up&&!previous.up,down=held->down&&!previous.down;
    int left=held->left&&!previous.left,right=held->right&&!previous.right;
    int a=held->a&&!previous.a,b=held->b&&!previous.b;
    int y=held->y&&!previous.y;
    int any=held->up||held->down||held->left||held->right;
    int repaint=slide_time<.266667f;
    if(up||down||left||right||a||b) {on_press(up,down,left,right,a,b);repeat_wait=.38f;repaint=1;}
    else if(any) {repeat_wait-=dt;if(repeat_wait<=0) {on_press(held->up,held->down,held->left,held->right,0,0);repeat_wait=.11f;repaint=1;}}
    if(slide_time<.266667f) {
        slide_time+=dt;
        if(slide_time>=.266667f && requested_blade!=blade) {
            direction=requested_blade>blade?1:-1;
            from_blade=blade;blade+=direction;slide_time=0;sound_event=UI_SOUND_BLADE;
        } else if(slide_time>=.266667f && pending_a) {pending_a=0;activate();}
    }
    if(page==MY_GAMES && fabsf(focus_vis-game_i)>.001f) {
        focus_vis+=(game_i-focus_vis)*fminf(1,dt*22);repaint=1;
        if(fabsf(focus_vis-game_i)<.01f)focus_vis=(float)game_i;
    }
    if(y && page==MY_GAMES) {
        unavailable("Xbox LIVE Marketplace","Xbox LIVE Marketplace is not available in this standalone launcher.");
        sound_event=UI_SOUND_SELECT;repaint=1;
    }
    previous=*held;
    if(repaint)paint();
}
