#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct plm_t;

namespace Framework {

    class VideoPlayer {
    public:
        VideoPlayer() = default;
        ~VideoPlayer();

        VideoPlayer(const VideoPlayer&) = delete;
        VideoPlayer& operator=(const VideoPlayer&) = delete;

        bool Load(const std::string& path);
        void Start();
        void Stop();
        void Update(float dt);
        void Draw() const;

        bool IsLoaded() const { return movie != nullptr; }
        bool IsFinished() const { return finished; }

    private:
        void DecodeAndUploadFrame();
        void FlipFrameVertically();
        void DestroyTexture();

        plm_t* movie = nullptr;
        unsigned textureId = 0;
        int width = 0;
        int height = 0;
        double frameDuration = 0.0;
        double accumulator = 0.0;
        bool finished = false;
        bool started = false;
        std::vector<std::uint8_t> rgbBuffer;
        std::vector<std::uint8_t> rowScratch;
    };

} // namespace Framework
