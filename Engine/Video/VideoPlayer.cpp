/*********************************************************************************************
 \file      VideoPlayer.cpp
 \par       SofaSpuds
 \author    h.jun (h.jun@digipen.edu) - Primary Author, 100%

 \brief     Implements a lightweight MPEG-1 video player using pl_mpeg and OpenGL textures.

 \details   This module provides simple cutscene/video playback support:
            - Video is decoded using pl_mpeg (CPU-side) into an RGB buffer.
            - Each decoded frame is uploaded into an OpenGL texture (GL_TEXTURE_2D).
            - Playback timing is driven by accumulated dt to step frames at the source FPS.
            - Audio is disabled (video-only playback).
            - Frames are vertically flipped before upload to match OpenGL texture orientation.

            Ownership / lifetime rules:
            - VideoPlayer owns the plm_t* movie handle (created by plm_create_with_filename).
            - VideoPlayer owns the OpenGL texture (textureId) used for display.
            - Stop() is responsible for releasing both resources safely.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "VideoPlayer.hpp"

#include "Graphics/Graphics.hpp"
#include <glad/glad.h>
#include <pl_mpeg.h>
#include <algorithm>

namespace Framework {

    /*************************************************************************************
      \brief Destructor. Ensures all video and GPU resources are released.
      \note  Calls Stop() so this class is safe against leaks even if user forgets.
    *************************************************************************************/
    VideoPlayer::~VideoPlayer()
    {
        Stop();
    }

    /*************************************************************************************
      \brief Loads an MPEG video file and prepares an OpenGL texture for frame upload.
      \param path File path to the video (e.g., .mpg).
      \return true if the video was loaded and initialized successfully; false otherwise.
      \note  This function:
            - Destroys any previously loaded video via Stop().
            - Disables audio playback in pl_mpeg.
            - Allocates the RGB frame buffer (width * height * 3).
            - Creates an OpenGL texture sized to the video resolution.
      \warning The video must be a format supported by pl_mpeg (typically MPEG-1).
    *************************************************************************************/
    bool VideoPlayer::Load(const std::string& path)
    {
        Stop(); // reset any previous state/resources

        movie = plm_create_with_filename(path.c_str());
        if (!movie) {
            return false;
        }

        // Video-only playback (no audio channel)
        plm_set_audio_enabled(movie, false);
        plm_set_loop(movie, false);

        // Query resolution from the stream
        width = plm_get_width(movie);
        height = plm_get_height(movie);
        if (width <= 0 || height <= 0) {
            Stop();
            return false;
        }

        // Timing: convert FPS to seconds-per-frame
        const double frameRate = plm_get_framerate(movie);
        frameDuration = (frameRate > 0.0) ? (1.0 / frameRate) : (1.0 / 30.0);

        accumulator = 0.0;
        finished = false;
        started = false;

        // CPU-side decoded RGB buffer (interleaved RGB24)
        rgbBuffer.assign(static_cast<size_t>(width * height * 3), 0);

        // GPU-side texture created once; updated per frame via glTexSubImage2D
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Allocate storage for texture (no initial pixel data)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    /*************************************************************************************
      \brief Starts playback from the beginning of the loaded movie.
      \note  If no movie is loaded, this call is ignored.
      \details
        - Rewinds the pl_mpeg decoder to frame 0.
        - Resets timers and finished state.
        - Decodes and uploads one frame immediately so Draw() has content right away.
    *************************************************************************************/
    void VideoPlayer::Start()
    {
        if (!movie) {
            return;
        }

        plm_rewind(movie);
        accumulator = 0.0;
        finished = false;
        started = true;

        // Prime the first frame so the screen isn't blank on start
        DecodeAndUploadFrame();
    }

    /*************************************************************************************
      \brief Stops playback and releases all owned resources.
      \note  Safe to call multiple times.
      \details
        - Destroys the pl_mpeg movie handle.
        - Deletes the OpenGL texture.
        - Clears buffers and resets state flags.
    *************************************************************************************/
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

    /*************************************************************************************
      \brief Advances video playback based on elapsed time (dt).
      \param dt Delta time in seconds since last update.
      \note  Playback is frame-stepped using accumulator >= frameDuration.
      \details
        - Accumulates dt (clamped to non-negative).
        - Decodes and uploads frames until we catch up to real time.
        - Stops stepping once the movie ends (finished becomes true).
    *************************************************************************************/
    void VideoPlayer::Update(float dt)
    {
        if (!movie || finished || !started) {
            return;
        }

        accumulator += std::max(0.0, static_cast<double>(dt));

        // If dt is large, we may need to decode multiple frames to catch up
        while (accumulator >= frameDuration && !finished) {
            DecodeAndUploadFrame();
            accumulator -= frameDuration;
        }
    }

    /*************************************************************************************
      \brief Draws the current decoded frame to the screen.
      \note  This assumes Graphics::renderFullscreenTexture() draws a full-screen quad.
      \details
        - If no texture is allocated, nothing is drawn.
        - The texture contains the most recently uploaded decoded frame.
    *************************************************************************************/
    void VideoPlayer::Draw() const
    {
        if (textureId) {
            gfx::Graphics::renderFullscreenTexture(textureId);
        }
    }

    /*************************************************************************************
      \brief Decodes the next video frame and uploads it into the OpenGL texture.
      \note  If decoding returns nullptr and the stream has ended, sets finished=true.
      \details
        - plm_decode_video() returns a decoded YCbCr frame from pl_mpeg.
        - plm_frame_to_rgb() converts it to interleaved RGB in rgbBuffer.
        - FlipFrameVertically() corrects orientation for OpenGL.
        - glTexSubImage2D() uploads the RGB pixels into the existing texture.
    *************************************************************************************/
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

        // Convert decoded frame into RGB24 buffer (stride = width * 3 bytes per row)
        plm_frame_to_rgb(frame, rgbBuffer.data(), width * 3);

        // OpenGL textures typically treat (0,0) as bottom-left; decoded buffers are top-left
        FlipFrameVertically();

        // Upload the frame into the existing GL texture
        glBindTexture(GL_TEXTURE_2D, textureId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /*************************************************************************************
      \brief Vertically flips the RGB buffer in-place.
      \note  Uses rowScratch as temporary storage for one scanline.
      \details
        - Swaps top and bottom rows moving inward until half the image height is processed.
        - Required because many decoders output top-left origin while OpenGL expects
          texture data in bottom-left origin for common screen-space mapping.
    *************************************************************************************/
    void VideoPlayer::FlipFrameVertically()
    {
        if (width <= 0 || height <= 1) {
            return;
        }

        const size_t rowBytes = static_cast<size_t>(width) * 3;

        // Allocate scratch row buffer if needed
        if (rowScratch.size() != rowBytes) {
            rowScratch.resize(rowBytes);
        }

        std::uint8_t* data = rgbBuffer.data();

        // Swap scanlines y and (height-1-y)
        for (int y = 0; y < height / 2; ++y) {
            std::uint8_t* top = data + static_cast<size_t>(y) * rowBytes;
            std::uint8_t* bottom = data + static_cast<size_t>(height - 1 - y) * rowBytes;

            std::copy(top, top + rowBytes, rowScratch.begin());
            std::copy(bottom, bottom + rowBytes, top);
            std::copy(rowScratch.begin(), rowScratch.end(), bottom);
        }
    }

    /*************************************************************************************
      \brief Deletes the OpenGL texture owned by this player, if any.
      \note  Safe to call multiple times.
    *************************************************************************************/
    void VideoPlayer::DestroyTexture()
    {
        if (textureId) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
    }

} // namespace Framework
