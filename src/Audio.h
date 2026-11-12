#pragma once

#include <memory>
#include <string>

// Sound effects and music: Ogg Vorbis clips decoded into memory up front and mixed on the audio thread.
// If no audio device can be opened, or a file cannot be read, the game carries on silently for that sound.
namespace audio {

class Mixer {
public:
    // Opens the default output device. Call it from the main thread, like everything else here.
    Mixer();
    ~Mixer();
    Mixer(const Mixer&) = delete;
    Mixer& operator=(const Mixer&) = delete;

    // Decodes an .ogg file and returns its clip id, or -1 if it cannot be read (a warning is printed).
    int load(const std::string& path);

    // Starts a clip. `volume` is 0..1, `pan` -1 (left) .. 1 (right). Ids of -1 are ignored, and a clip restarted
    // within a few milliseconds of itself is skipped, so a burst of identical hits does not stack up.
    void play(int clip, float volume = 1.0f, float pan = 0.0f);
    // Plays a clip over and over until stop() (background music). It is never cut off for other sounds.
    void loop(int clip, float volume);
    // Stops every voice playing `clip`.
    void stop(int clip);

    void setMuted(bool muted);
    bool muted() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace audio
