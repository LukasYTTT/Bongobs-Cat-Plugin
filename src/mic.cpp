#include "header.hpp"
#include <atomic>
#include <cmath>

namespace mic {

std::atomic<bool> is_talking_flag(false);

class CatMicRecorder : public sf::SoundRecorder {
public:
    CatMicRecorder() {
        threshold = 0.05f;
    }
    void setThreshold(float t) {
        threshold = t;
    }
protected:
    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) override {
        double sum = 0;
        for (std::size_t i = 0; i < sampleCount; ++i) {
            float sample = samples[i] / 32768.0f;
            sum += sample * sample;
        }
        double rms = std::sqrt(sum / sampleCount);
        is_talking_flag = (rms > threshold);
        return true; // continue recording
    }
private:
    float threshold;
};

CatMicRecorder* recorder = nullptr;

bool init() {
    // Wenn in der Config kein mic setup ist, erstelle es
    if (!data::cfg["osu"].isMember("mic")) {
        data::cfg["osu"]["mic"]["enabled"] = true; // DEFAULT ON
        data::cfg["osu"]["mic"]["threshold"] = 0.05f;
        data::cfg["osu"]["mic"]["mouthOffsetX"] = 320; // ungefähre Mitte der originalen Katze
        data::cfg["osu"]["mic"]["mouthOffsetY"] = 170;
        data::cfg["osu"]["mic"]["mouthWidth"] = 12;
        data::cfg["osu"]["mic"]["mouthHeight"] = 10;
        data::save_config();
    }

    if (!data::cfg["osu"]["mic"]["enabled"].asBool()) {
        clean();
        return false;
    }
    
    if (!sf::SoundRecorder::isAvailable()) {
        std::cout << "Microphone is not available on this system." << std::endl;
        return false;
    }

    if (!recorder) {
        recorder = new CatMicRecorder();
        float threshold = data::cfg["osu"]["mic"]["threshold"].asFloat();
        recorder->setThreshold(threshold);
        recorder->start();
    }
    
    return true;
}

void clean() {
    if (recorder) {
        recorder->stop();
        delete recorder;
        recorder = nullptr;
    }
}

bool is_talking() {
    return is_talking_flag.load();
}

}; // namespace mic
