/* Rise of the Triad (Taradino) on the MiSTer: the music through jfaudiolib's MiSTer driver, as a Taradino
 * music module (music.h music_module_t; dukemusc.c picks it as MusicMode 1 in place of SDL_mixer's, which has no
 * MIDI synthesizer on the MiSTer). jfaudiolib's music.c is linked with its MUSIC_* symbols renamed JFA_*
 * (objcopy --redefine-syms, taradino_build.sh) because Taradino's music.h uses the same names. The driver's clock
 * is SDL_mixer's music hook: the hook fills silence (the music comes from the core) and advances the driver by
 * the frames the callback asked for. 2026-11-25. */
#include <stddef.h>
#include <string.h>
#include "SDL.h"
#include "SDL_mixer.h"
#include "music.h"

enum { JFA_ASS_MisterFM = 8 };   /* sndcards.h of jfaudiolib: ASS_NoSound 0 .. ASS_MisterFM (checked at build) */
extern int  JFA_MUSIC_Init(int SoundCard, const char *params);
extern int  JFA_MUSIC_Shutdown(void);
extern void JFA_MUSIC_SetVolume(int volume);
extern int  JFA_MUSIC_SongPlaying(void);
extern void JFA_MUSIC_Continue(void);
extern void JFA_MUSIC_Pause(void);
extern int  JFA_MUSIC_StopSong(void);
extern int  JFA_MUSIC_PlaySong(void *song, unsigned int length, int loopflag);
extern int  JFA_MUSIC_FadeVolume(int tovolume, int milliseconds);
extern int  JFA_MUSIC_FadeActive(void);
void MisterFMDrv_Advance(int frames, int rate);

static int mm_started = 0, mm_rate = 44100, mm_framebytes = 4;

static void mm_clock(void *udata, Uint8 *stream, int len)
{
	(void)udata;
	memset(stream, 0, len);
	if (mm_started) MisterFMDrv_Advance(len / mm_framebytes, mm_rate);
}

static int MM_Init(int samplerate)
{
	int f, c; Uint16 fmt;
	mm_rate = samplerate;
	if (Mix_QuerySpec(&f, &fmt, &c)) { mm_rate = f; mm_framebytes = c * ((fmt & 0xff) / 8); }
	if (!mm_started)
	{
		if (JFA_MUSIC_Init(JFA_ASS_MisterFM, "") != MUSIC_Ok) return MUSIC_Error;
		mm_started = 1;
	}
	Mix_HookMusic(mm_clock, NULL);
	return MUSIC_Ok;
}

static int MM_Shutdown(void)
{
	Mix_HookMusic(NULL, NULL);
	if (mm_started) { JFA_MUSIC_Shutdown(); mm_started = 0; }
	return MUSIC_Ok;
}

static int  MM_SongPlaying(void) { return mm_started ? JFA_MUSIC_SongPlaying() : 0; }
static void MM_Continue(void) { if (mm_started) JFA_MUSIC_Continue(); }
static void MM_Pause(void) { if (mm_started) JFA_MUSIC_Pause(); }
static int  MM_StopSong(void) { return mm_started ? JFA_MUSIC_StopSong() : MUSIC_Ok; }
static int  MM_PlaySong(unsigned char *song, int size, int loopflag)
{
	if (!mm_started || song == NULL || size <= 0) return MUSIC_Error;
	return JFA_MUSIC_PlaySong(song, (unsigned int)size, loopflag == MUSIC_LoopSong);
}
static int  MM_FadeVolume(int tovolume, int milliseconds) { return mm_started ? JFA_MUSIC_FadeVolume(tovolume, milliseconds) : MUSIC_Ok; }
static int  MM_FadeActive(void) { return mm_started ? JFA_MUSIC_FadeActive() : 0; }

/* dukemusc.c MUSIC_SetVolume(): the game's 0..255 */
void mister_music_setvolume(int volume) { if (mm_started) JFA_MUSIC_SetVolume(volume); }

music_module_t mister_music_module = {
	MM_Init, MM_Shutdown, MM_SongPlaying, MM_Continue, MM_Pause, MM_StopSong, MM_PlaySong, MM_FadeVolume, MM_FadeActive,
};
