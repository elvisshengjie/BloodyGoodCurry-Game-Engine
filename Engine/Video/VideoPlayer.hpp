/*********************************************************************************************
 \file      VideoPlayer.hpp
 \par       SofaSpuds
 \author    h.jun (h.jun@digipen.edu) - Primary Author, 100%

 \brief     Declares a lightweight MPEG video player (video-only) backed by pl_mpeg and OpenGL.

 \details   VideoPlayer is a small utility used for cutscenes / fullscreen playback:
            - Decodes MPEG-1 frames using pl_mpeg (CPU-side).
            - Converts decoded frames to RGB and uploads them to an OpenGL texture.
            - Playback timing is controlled using dt accumulation against the movie FPS.
            - Audio is disabled (video-only).
            - The decoded frame buffer is vertically flipped before upload to match common
              OpenGL texture orientation.

            Ownership / lifetime rules:
            - The class owns the pl_mpeg movie handle (plm_t*) and destroys it in Stop()/~VideoPlayer().
            - The class owns the OpenGL texture (textureId) used for the current frame.
            - Load() implicitly calls Stop() first to release any previous resources.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct plm_t;

namespace Framework {

    /*************************************************************************************
      \class VideoPlayer
      \brief Plays a video by decoding frames and rendering them as a fullscreen texture.
      \details
        Typical usage:
          - player.Load("Data_Files/Cutscenes/intro.mpg");
          - player.Start();
          - In game loop: player.Update(dt); player.Draw();
          - Query completion with IsFinished() to transition states.

        Notes:
          - This class is non-copyable because it owns GPU and decoder resources.
          - Draw() depends on the engine graphics helper renderFullscreenTexture().
    *************************************************************************************/
    class VideoPlayer {
    public:
        /*************************************************************************************
          \brief Default constructor. Creates an empty player (no movie loaded).
        *************************************************************************************/
        VideoPlayer() = default;

        /*************************************************************************************
          \brief Destructor. Ensures all resources are released.
          \note  Equivalent to calling Stop().
        *************************************************************************************/
        ~VideoPlayer();

        /*************************************************************************************
          \brief Deleted copy constructor.
          \note  VideoPlayer owns unique resources (decoder handle + GL texture).
        *************************************************************************************/
        VideoPlayer(const VideoPlayer&) = delete;

        /*************************************************************************************
          \brief Deleted copy assignment.
          \note  VideoPlayer owns unique resources (decoder handle + GL texture).
        *************************************************************************************/
        VideoPlayer& operator=(const VideoPlayer&) = delete;

        /*************************************************************************************
          \brief Loads an MPEG video file and allocates an OpenGL texture for it.
          \param path Path to the video file to load.
          \return true on success; false if the file cannot be opened/decoded or is invalid.
          \note  This call resets any previously loaded video (calls Stop()).
        *************************************************************************************/
        bool Load(const std::string& path);

        /*************************************************************************************
          \brief Starts playback from the beginning of the loaded movie.
          \note  If no movie is loaded, this call has no effect.
          \details
            - Rewinds the decoder and resets timing.
            - Decodes and uploads the first frame immediately so Draw() has content.
        *************************************************************************************/
        void Start();

        /*************************************************************************************
          \brief Stops playback and releases decoder + GPU resources.
          \note  Safe to call multiple times.
        *************************************************************************************/
        void Stop();

        /*************************************************************************************
          \brief Advances playback by dt seconds and uploads any required frames.
          \param dt Delta time in seconds since the last update.
          \note  Frames advance when the internal accumulator exceeds frameDuration.
        *************************************************************************************/
        void Update(float dt);

        /*************************************************************************************
          \brief Renders the most recently decoded frame to the screen.
          \note  Does nothing if no texture exists (not loaded / already stopped).
        *************************************************************************************/
        void Draw() const;

        void SetColorKeyEnabled(bool enabled, float thresholdLow = 0.02f, float thresholdHigh = 0.10f);

        /*************************************************************************************
          \brief Returns whether a movie is currently loaded.
          \return true if the internal decoder handle exists; false otherwise.
        *************************************************************************************/
        bool IsLoaded() const { return movie != nullptr; }

        /*************************************************************************************
          \brief Returns whether playback has finished (movie reached end).
          \return true if the video stream ended; false otherwise.
        *************************************************************************************/
        bool IsFinished() const { return finished; }

    private:
        /*************************************************************************************
          \brief Decodes the next frame from the stream and uploads it to the GL texture.
          \note  Sets finished=true if decoding reaches the end of the stream.
        *************************************************************************************/
        void DecodeAndUploadFrame();

        /*************************************************************************************
          \brief Flips the RGB frame buffer vertically in-place.
          \details
            - Many decoders output top-left origin, while typical OpenGL texture sampling
              for screen-space expects bottom-left origin.
            - Uses rowScratch as temporary storage for one scanline swap.
        *************************************************************************************/
        void FlipFrameVertically();

        /*************************************************************************************
          \brief Deletes the owned OpenGL texture (if it exists) and resets textureId.
        *************************************************************************************/
        void DestroyTexture();

        // Decoder state (owned by this class; created/destroyed via pl_mpeg API)
        plm_t* movie = nullptr;

        // OpenGL texture storing the most recently decoded frame (owned by this class)
        unsigned textureId = 0;

        // Video resolution queried from the decoder
        int width = 0;
        int height = 0;

        // Timing control
        double frameDuration = 0.0;   // seconds per frame (1 / fps)
        double accumulator = 0.0;     // time accumulator used for stepping frames

        // Playback state
        bool finished = false;        // set when stream reaches end
        bool started = false;         // true after Start() is called
        bool colorKeyEnabled = false;
        float colorKeyThresholdLow = 0.02f;
        float colorKeyThresholdHigh = 0.10f;

        // CPU-side buffers
        std::vector<std::uint8_t> rgbBuffer;  // RGB24 pixel buffer (width * height * 3)
        std::vector<std::uint8_t> rowScratch; // temporary row storage for vertical flip
    };

} // namespace Framework
