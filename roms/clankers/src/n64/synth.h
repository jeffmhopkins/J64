// The N64 sound: a software synth fed by the core's sound effects and songs.
#ifndef CK_SYNTH_H
#define CK_SYNTH_H

#include <stdbool.h>

void synth_init(const char *songs_text);   // songs.txt contents (may be NULL)
void synth_sfx(int id);                     // SFX_*
void synth_song(int id);                    // SONG_*; SONG_NONE stops the music
void synth_poll(void);                      // call once per frame
void synth_mute(bool muted);

#endif
