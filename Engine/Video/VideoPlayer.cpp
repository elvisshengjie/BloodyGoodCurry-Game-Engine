#include "VideoPlayer.hpp"

#include "Graphics/Graphics.hpp"
#include <glad/glad.h>
#include <pl_mpeg.h>
#include <algorithm>

namespace Framework {

    VideoPlayer::~VideoPlayer()
    {
        Stop();
    }

    bool VideoPlayer::Load(const std::string& path)
    {
        Stop();

        movie = plm_create_with_filename(path.c_str());
        if (!movie) {
            return false;
        }

        plm_set_audio_enabled(movie, false);
        plm_set_loop(movie, false);

        width = plm_get_width(movie);
        height = plm_get_height(movie);
        if (width <= 0 || height <= 0) {
            Stop();
            return false;
        }

        const double frameRate = plm_get_framerate(movie);
        frameDuration = (frameRate > 0.0) ? (1.0 / frameRate) : (1.0 / 30.0);
        accumulator = 0.0;
        finished = false;
        started = false;

        rgbBuffer.assign(static_cast<size_t>(width * height * 3), 0);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    void VideoPlayer::Start()
    {
        if (!movie) {
            return;
        }

        plm_rewind(movie);
        accumulator = 0.0;
        finished = false;
        started = true;
        DecodeAndUploadFrame();
    }

    void VideoPlayer::Stop()
    {
        if (movie) {
            plm_destroy(movie);
            movie = nullptr;
        }
        DestroyTexture();
        rgbBuffer.clear();
        width = 0;
        height = 0;
        frameDuration = 0.0;
        accumulator = 0.0;
        finished = false;
        started = false;
    }

    void VideoPlayer::Update(float dt)
    {
        if (!movie || finished || !started) {
            return;
        }

        accumulator += std::max(0.0, static_cast<double>(dt));
        while (accumulator >= frameDuration && !finished) {
            DecodeAndUploadFrame();
            accumulator -= frameDuration;
        }
    }

    void VideoPlayer::Draw() const
    {
        if (textureId) {
            gfx::Graphics::renderFullscreenTexture(textureId);
        }
    }

    void VideoPlayer::DecodeAndUploadFrame()
    {
        if (!movie) {
            return;
        }

        plm_frame_t* frame = plm_decode_video(movie);
        if (!frame) {
            if (plm_has_ended(movie)) {
                finished = true;
            }
            return;
        }

        plm_frame_to_rgb(frame, rgbBuffer.data(), width * 3);
        FlipFrameVertically();
        glBindTexture(GL_TEXTURE_2D, textureId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void VideoPlayer::FlipFrameVertically()
    {
        if (width <= 0 || height <= 1) {
            return;
        }

        const size_t rowBytes = static_cast<size_t>(width) * 3;
        if (rowScratch.size() != rowBytes) {
            rowScratch.resize(rowBytes);
        }

        std::uint8_t* data = rgbBuffer.data();
        for (int y = 0; y < height / 2; ++y) {
            std::uint8_t* top = data + static_cast<size_t>(y) * rowBytes;
            std::uint8_t* bottom = data + static_cast<size_t>(height - 1 - y) * rowBytes;
            std::copy(top, top + rowBytes, rowScratch.begin());
            std::copy(bottom, bottom + rowBytes, top);
            std::copy(rowScratch.begin(), rowScratch.end(), bottom);
        }
    }

    void VideoPlayer::DestroyTexture()
    {
        if (textureId) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
    }

} // namespace Framework
