#ifndef BLADES_UI_AUDIO_H
#define BLADES_UI_AUDIO_H
/* Event numbers are shared by the app and both native platform adapters. */
enum { UI_SOUND_NONE, UI_SOUND_MOVE, UI_SOUND_BLADE, UI_SOUND_SELECT, UI_SOUND_BACK, UI_SOUND_GUIDE_OPEN, UI_SOUND_GUIDE_CLOSE };
typedef struct { const short *samples; unsigned int count; } UiAudioClip;
extern const UiAudioClip ui_audio_clips[5];
#endif
