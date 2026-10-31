#include "blades.h"
#include "ui_audio.h"
#include "guide.h"
#include "stick.h"
#include "draw.h"
#include "library.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint64_t fingerprint(void)
{
    const uint32_t *p=blades_pixels();uint64_t hash=UINT64_C(1469598103934665603);
    for(int i=0;i<BLADES_WIDTH*BLADES_HEIGHT;i++) hash=(hash^p[i])*UINT64_C(1099511628211);
    return hash;
}
static void settle(void) { for(int i=0;i<80;i++) blades_frame(&(BladesHeld){0},1.0f/60); }
static void tap(BladesHeld held) { blades_frame(&held,1.0f/60);settle(); }
static void guide_checks(void)
{
    blades_init();blades_set_clock(4,8);
    uint64_t home=fingerprint();
    tap((BladesHeld){.guide=1});uint64_t root=fingerprint();
    assert(root!=home&&guide_active());
    assert(blades_take_sound()==UI_SOUND_GUIDE_OPEN);
    tap((BladesHeld){.a=1});uint64_t signin=fingerprint();assert(signin!=root);
    tap((BladesHeld){.a=1});assert(fingerprint()!=signin);
    tap((BladesHeld){.b=1});assert(fingerprint()==signin);
    tap((BladesHeld){.b=1});assert(fingerprint()==root);
    /* Actual status replaces the captured profile count; no account is invented. */
    blades_set_user("TestPlayer");blades_frame(&(BladesHeld){0},0);assert(fingerprint()!=root);
    blades_set_user(NULL);blades_frame(&(BladesHeld){0},0);assert(fingerprint()==root);
    /* Root B closes, while nested B returns one page. */
    tap((BladesHeld){.b=1});assert(!guide_active()&&fingerprint()==home);
    assert(blades_take_sound()==UI_SOUND_GUIDE_CLOSE);
    /* Auto-repeat clamps at the last root control and never activates it. */
    tap((BladesHeld){.guide=1});
    for(int i=0;i<100;i++)blades_frame(&(BladesHeld){.down=1},1.0f/60);
    settle();uint64_t last_control=fingerprint();
    tap((BladesHeld){.down=1});assert(fingerprint()==last_control);
    tap((BladesHeld){.a=1});uint64_t confirmation=fingerprint();
    tap((BladesHeld){.b=1});assert(fingerprint()==last_control);
    tap((BladesHeld){.y=1});assert(fingerprint()==confirmation);
    tap((BladesHeld){.a=1});assert(fingerprint()==last_control);
    tap((BladesHeld){.x=1});assert(fingerprint()!=last_control);
    tap((BladesHeld){.b=1});assert(fingerprint()==last_control);
    for(int i=0;i<100;i++)blades_frame(&(BladesHeld){.up=1},1.0f/60);
    settle();assert(fingerprint()==root);
    tap((BladesHeld){.b=1});assert(fingerprint()==home);
    tap((BladesHeld){.a=1});tap((BladesHeld){.a=1});
    tap((BladesHeld){.down=1});uint64_t game=fingerprint();
    tap((BladesHeld){.guide=1});
    /* Main navigation and both selected music controls remain inside the Guide. */
    for(int i=0;i<4;i++)tap((BladesHeld){.down=1});
    uint64_t music=fingerprint();tap((BladesHeld){.right=1});assert(fingerprint()!=music);
    tap((BladesHeld){.left=1});assert(fingerprint()==music);
    tap((BladesHeld){.a=1});assert(fingerprint()!=music);
    tap((BladesHeld){.b=1});assert(fingerprint()==music);
    /* Holding B through the close animation cannot also leave My Games. */
    for(int i=0;i<80;i++)blades_frame(&(BladesHeld){.b=1},1.0f/60);
    assert(!guide_active()&&fingerprint()==game);
    settle();
    /* Closing with a held d-pad cannot scroll the game list after the panel leaves. */
    tap((BladesHeld){.guide=1});
    blades_frame(&(BladesHeld){.guide=1,.down=1},1.0f/60);
    for(int i=0;i<100;i++)blades_frame(&(BladesHeld){.down=1},1.0f/60);
    assert(!guide_active()&&fingerprint()==game);
    blades_frame(&(BladesHeld){0},1.0f/60);
    tap((BladesHeld){.guide=1});assert(guide_active());
    tap((BladesHeld){.b=1});assert(fingerprint()==game);
    /* Default No cancels dashboard confirmation; explicit Yes returns home. */
    tap((BladesHeld){.guide=1});uint64_t game_root=fingerprint();
    tap((BladesHeld){.y=1});assert(fingerprint()!=game_root);
    tap((BladesHeld){.a=1});assert(fingerprint()==game_root);
    tap((BladesHeld){.b=1});assert(fingerprint()==game);
    tap((BladesHeld){.guide=1});tap((BladesHeld){.y=1});
    tap((BladesHeld){.up=1});tap((BladesHeld){.a=1});
    assert(!guide_active()&&fingerprint()==home);
    /* Reverse an opening/closing animation without leaving a dimmed framebuffer. */
    tap((BladesHeld){.guide=1});
    blades_frame(&(BladesHeld){.guide=1},.016f);
    blades_frame(&(BladesHeld){0},.016f);
    blades_frame(&(BladesHeld){.guide=1},.016f);settle();
    assert(guide_active());tap((BladesHeld){.b=1});assert(fingerprint()==home);
    /* Freeze a partially completed blade fold under the Guide. */
    blades_init();blades_frame(&(BladesHeld){.right=1},.016f);
    blades_frame(&(BladesHeld){0},.016f);uint64_t folded=fingerprint();
    tap((BladesHeld){.guide=1});
    blades_frame(&(BladesHeld){.b=1},1.0f/60);
    int frames=0;
    while(guide_active()&&frames++<120)blades_frame(&(BladesHeld){0},1.0f/60);
    assert(!guide_active()&&fingerprint()==folded);
    settle();assert(fingerprint()!=folded);
    /* XAM takeover dismisses our overlay and blocks held input until release. */
    blades_init();home=fingerprint();tap((BladesHeld){.guide=1});
    blades_system_ui_state(1);assert(!guide_active()&&fingerprint()==home);
    for(int i=0;i<20;i++)blades_frame(&(BladesHeld){.a=1,.right=1},.016f);
    assert(fingerprint()==home);
    blades_system_ui_state(0);
    for(int i=0;i<20;i++)blades_frame(&(BladesHeld){.a=1},.016f);
    assert(fingerprint()==home);
    settle();tap((BladesHeld){.a=1});assert(fingerprint()!=home);
    blades_init();assert(!guide_active());
}
int main(void)
{
    BladesStick stick={0};BladesHeld analog={0};
    blades_stick_apply(&analog,&stick,7000,0);assert(!analog.right);
    blades_stick_apply(&analog,&stick,14000,0);assert(analog.right);
    analog=(BladesHeld){0};blades_stick_apply(&analog,&stick,9000,0);assert(analog.right);
    analog=(BladesHeld){0};blades_stick_apply(&analog,&stick,7000,0);assert(!analog.right);
    analog=(BladesHeld){0};blades_stick_apply(&analog,&stick,-32768,12000);assert(analog.left&&!analog.up);
    analog=(BladesHeld){.right=1};blades_stick_apply(&analog,&stick,12000,32767);assert(analog.up&&analog.right&&!analog.left);
    /* Fast dimming must remain pixel-identical to black source-over. */
    uint32_t sample[256];
    for(int alpha=0;alpha<256;alpha++) {
        for(int i=0;i<256;i++)sample[i]=0xFF000000u|((uint32_t)i<<16)|((uint32_t)(255-i)<<8)|i;
        draw_target(sample,256,1);draw_dim(alpha);
        for(int i=0;i<256;i++)assert(sample[i]==(0xFF000000u|((uint32_t)(i*(255-alpha)/255)<<16)|((uint32_t)((255-i)*(255-alpha)/255)<<8)|(uint32_t)(i*(255-alpha)/255)));
    }
    draw_target(NULL,0,0);
    blades_init();uint64_t home=fingerprint();
    tap((BladesHeld){.a=1});uint64_t library=fingerprint();assert(library!=home);
    tap((BladesHeld){.a=1});uint64_t games=fingerprint();assert(games!=library);
    tap((BladesHeld){.down=1});uint64_t second=fingerprint();assert(second!=games);
    assert(blades_take_sound()==UI_SOUND_MOVE);assert(blades_take_sound()==UI_SOUND_NONE);
    tap((BladesHeld){.a=1});assert(fingerprint()!=second);
    tap((BladesHeld){.b=1});assert(fingerprint()==second);
    for(int i=0;i<12;i++)tap((BladesHeld){.down=1});
    uint64_t last=fingerprint();tap((BladesHeld){.down=1});assert(fingerprint()==last);
    for(int i=0;i<12;i++)tap((BladesHeld){.up=1});assert(fingerprint()==games);
    tap((BladesHeld){.right=1});assert(fingerprint()!=games); /* Arcade filter */
    tap((BladesHeld){.left=1});assert(fingerprint()==games);
    tap((BladesHeld){.b=1});assert(fingerprint()==library);
    tap((BladesHeld){.b=1});assert(fingerprint()==home);
    /* Both new left-side blades exist, and navigation clamps at Marketplace. */
    tap((BladesHeld){.left=1});uint64_t live=fingerprint();assert(live!=home);
    tap((BladesHeld){.left=1});uint64_t market=fingerprint();assert(market!=live);
    tap((BladesHeld){.left=1});assert(fingerprint()==market);
    tap((BladesHeld){.right=1});assert(fingerprint()==live);
    tap((BladesHeld){.right=1});assert(fingerprint()==home);
    tap((BladesHeld){.right=1});uint64_t media=fingerprint();assert(media!=home);
    tap((BladesHeld){.right=1});uint64_t system=fingerprint();assert(system!=media);
    tap((BladesHeld){.right=1});assert(fingerprint()==system);
    tap((BladesHeld){.a=1});uint64_t console=fingerprint();assert(console!=system);
    tap((BladesHeld){.a=1});assert(fingerprint()!=console);
    tap((BladesHeld){.b=1});assert(fingerprint()==console);
    tap((BladesHeld){.down=1});uint64_t audio_selection=fingerprint();
    tap((BladesHeld){.a=1});assert(fingerprint()!=audio_selection);
    tap((BladesHeld){.b=1});assert(fingerprint()==audio_selection);
    tap((BladesHeld){.b=1});assert(fingerprint()==system);
    /* Rapid reversal queues a fold rather than teleporting an in-flight blade. */
    blades_init();blades_frame(&(BladesHeld){.right=1},.016f);
    blades_frame(&(BladesHeld){0},.016f);
    blades_frame(&(BladesHeld){.left=1},.016f);settle();assert(fingerprint()==home);
    /* A queued during the fold activates the incoming blade after settling. */
    blades_init();blades_frame(&(BladesHeld){.right=1},.016f);
    blades_frame(&(BladesHeld){0},.016f);blades_frame(&(BladesHeld){.a=1},.016f);settle();
    assert(fingerprint()!=media);tap((BladesHeld){.b=1});assert(fingerprint()==media);
    blades_frame(&(BladesHeld){.down=1},.016f);blades_init();assert(fingerprint()==home);
    guide_checks();
    /* Installed catalog replaces samples, scrolls past eight rows, and handles
     * empty GOD-inapplicable filters without indexing the sample icon array. */
    blades_init();BladesGame catalog[20]={0};
    for(int i=0;i<20;i++)snprintf(catalog[i].name,sizeof catalog[i].name,"Installed Game %02d",i);
    blades_set_library(catalog,20);blades_set_user("ConsolePlayer");
    tap((BladesHeld){.a=1});tap((BladesHeld){.a=1});
    for(int i=0;i<30;i++)tap((BladesHeld){.down=1});
    uint64_t bottom=fingerprint();tap((BladesHeld){.down=1});assert(fingerprint()==bottom);
    tap((BladesHeld){.right=1});uint64_t empty=fingerprint();tap((BladesHeld){.a=1});assert(fingerprint()==empty);
    tap((BladesHeld){.left=1});assert(fingerprint()!=empty);
    tap((BladesHeld){.guide=1});tap((BladesHeld){.b=1});
    blades_init();blades_set_library(NULL,-1);tap((BladesHeld){.a=1});tap((BladesHeld){.a=1});
    empty=fingerprint();tap((BladesHeld){.down=1});tap((BladesHeld){.a=1});assert(fingerprint()==empty);
    puts("Installed GOD catalog scrolling, empty categories and unavailable HDD passed.");
    puts("Guide focus, hierarchy, confirmation, input isolation, fold freeze and simulated system-UI gating passed.\nFive blades, hierarchy, tabs, selection limits, dialogs/back, queued folds, audio events and reset passed.");
}
