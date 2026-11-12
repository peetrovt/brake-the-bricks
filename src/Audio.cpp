#include "Audio.h"

#include "miniaudio/miniaudio.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <vector>

namespace audio {
namespace {

constexpr ma_uint32 kSampleRate = 44100;
constexpr ma_uint32 kChannels = 2;
constexpr size_t kMaxVoices = 32;
constexpr float kMasterVolume = 0.8f;
constexpr double kMinRestartSeconds = 0.035;

struct Clip {
    std::vector<float> samples;  // interleaved stereo at kSampleRate
    size_t frames = 0;
    std::chrono::steady_clock::time_point lastStart{};
};

struct Voice {
    int clip = -1;  // -1: free
    size_t position = 0;  // in frames
    float left = 1.0f;
    float right = 1.0f;
    bool looping = false;
    unsigned long long order = 0;  // when it started, to find the oldest one
};

// Stereo 16-bit (or mono, duplicated) at any rate -> stereo float at kSampleRate, by linear interpolation.
std::vector<float> toStereoFloat(const short* data, int frames, int channels, int rate) {
    auto sample = [&](int frame, int channel) {
        frame = std::min(frame, frames - 1);
        return data[frame * channels + std::min(channel, channels - 1)] / 32768.0f;
    };
    const double step = static_cast<double>(rate) / kSampleRate;
    const auto outFrames = static_cast<size_t>(frames / step);
    std::vector<float> out(outFrames * kChannels);
    for (size_t i = 0; i < outFrames; ++i) {
        const double source = i * step;
        const int index = static_cast<int>(source);
        const auto t = static_cast<float>(source - index);
        for (ma_uint32 c = 0; c < kChannels; ++c) {
            out[i * kChannels + c] = sample(index, c) * (1.0f - t) + sample(index + 1, c) * t;
        }
    }
    return out;
}

}  // namespace

struct Mixer::Impl {
    ma_device device{};
    bool deviceOpen = false;
    std::atomic<bool> muted{false};

    // Clips only grow, and only before they are played; voices are shared with the audio thread under `lock`.
    std::vector<Clip> clips;
    std::mutex lock;
    std::vector<Voice> voices = std::vector<Voice>(kMaxVoices);
    unsigned long long started = 0;

    static void callback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount) {
        static_cast<Impl*>(device->pUserData)->mix(static_cast<float*>(output), frameCount);
    }

    void mix(float* out, ma_uint32 frameCount) {
        std::fill(out, out + frameCount * kChannels, 0.0f);
        std::lock_guard<std::mutex> guard(lock);
        for (Voice& voice : voices) {
            if (voice.clip < 0) {
                continue;
            }
            const Clip& clip = clips[voice.clip];
            for (ma_uint32 i = 0; i < frameCount; ++i) {
                if (voice.position >= clip.frames) {
                    if (!voice.looping || clip.frames == 0) {
                        voice.clip = -1;
                        break;
                    }
                    voice.position = 0;
                }
                const float* frame = &clip.samples[voice.position * kChannels];
                out[i * kChannels] += frame[0] * voice.left;
                out[i * kChannels + 1] += frame[1] * voice.right;
                ++voice.position;
            }
        }
        // Muting keeps the voices running, so nothing stale resumes when the sound comes back.
        const float master = muted ? 0.0f : kMasterVolume;
        for (ma_uint32 i = 0; i < frameCount * kChannels; ++i) {
            out[i] = std::clamp(out[i] * master, -1.0f, 1.0f);
        }
    }

    void start(int clip, float volume, float pan, bool looping) {
        if (!deviceOpen || clip < 0 || clip >= static_cast<int>(clips.size())) {
            return;
        }
        pan = std::clamp(pan, -1.0f, 1.0f);
        std::lock_guard<std::mutex> guard(lock);
        // A free voice, else the oldest one that is not music.
        Voice* slot = nullptr;
        for (Voice& voice : voices) {
            if (voice.clip < 0) {
                slot = &voice;
                break;
            }
            if (!voice.looping && (!slot || voice.order < slot->order)) {
                slot = &voice;
            }
        }
        if (!slot) {
            return;
        }
        *slot = {clip, 0, volume * std::min(1.0f, 1.0f - pan), volume * std::min(1.0f, 1.0f + pan), looping, ++started};
    }
};

Mixer::Mixer() : impl_(std::make_unique<Impl>()) {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = kChannels;
    config.sampleRate = kSampleRate;
    config.dataCallback = &Impl::callback;
    config.pUserData = impl_.get();
    if (ma_device_init(nullptr, &config, &impl_->device) != MA_SUCCESS) {
        std::cerr << "Audio: no output device, playing without sound\n";
        return;
    }
    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        std::cerr << "Audio: could not start the output device, playing without sound\n";
        ma_device_uninit(&impl_->device);
        return;
    }
    impl_->deviceOpen = true;
}

Mixer::~Mixer() {
    if (impl_->deviceOpen) {
        ma_device_uninit(&impl_->device);  // stops the audio thread before the clips go away
    }
}

int Mixer::load(const std::string& path) {
    if (!impl_->deviceOpen) {
        return -1;
    }
    int channels = 0;
    int rate = 0;
    short* data = nullptr;
    const int frames = stb_vorbis_decode_filename(path.c_str(), &channels, &rate, &data);
    if (frames <= 0 || !data) {
        std::cerr << "Audio: cannot read " << path << '\n';
        std::free(data);
        return -1;
    }
    Clip clip;
    clip.samples = toStereoFloat(data, frames, channels, rate);
    clip.frames = clip.samples.size() / kChannels;
    std::free(data);

    std::lock_guard<std::mutex> guard(impl_->lock);
    impl_->clips.push_back(std::move(clip));
    return static_cast<int>(impl_->clips.size()) - 1;
}

void Mixer::play(int clip, float volume, float pan) {
    if (clip < 0 || clip >= static_cast<int>(impl_->clips.size())) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    auto& lastStart = impl_->clips[clip].lastStart;
    if (now - lastStart < std::chrono::duration<double>(kMinRestartSeconds)) {
        return;
    }
    lastStart = now;
    impl_->start(clip, volume, pan, false);
}

void Mixer::loop(int clip, float volume) {
    impl_->start(clip, volume, 0.0f, true);
}

void Mixer::stop(int clip) {
    std::lock_guard<std::mutex> guard(impl_->lock);
    for (Voice& voice : impl_->voices) {
        if (voice.clip == clip) {
            voice.clip = -1;
        }
    }
}

void Mixer::setMuted(bool muted) {
    impl_->muted = muted;
}

bool Mixer::muted() const {
    return impl_->muted;
}

}  // namespace audio
