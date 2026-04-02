/*********************************************************************************************
 \file      GUISystem.hpp
 \par       SofaSpuds
 \author    erika.ishii (erika.ishii@digipen.edu) - Primary Author, 100%
 \brief     Immediate-mode GUI for clickable buttons (text or textured).
 \details   Manages a flat list of buttons, updates hover and rising-edge click state from
            InputSystem, and renders via RenderSystem. Absolute positioning is used; the
            renderer interprets coordinates and draws rectangles or textures as needed.
 \copyright
            All content (c)2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Framework {
    class InputSystem;
    class RenderSystem;
}

/*************************************************************************************
   \class  GUISystem
   \brief  Stores and updates a flat list of screen-space buttons.
   \details Buttons may be text-only or texture-backed. Hover and click state are
            sampled each frame, then click callbacks are dispatched after a short
            visual press delay so menu interactions feel responsive.
 *************************************************************************************/
class GUISystem {
public:
    /*************************************************************************************
      \struct Button
      \brief  Lightweight button record used by GUISystem.
      \details Coordinates are stored in screen space using a bottom-left origin so
               they can be passed directly to the UI rendering helpers.
    *************************************************************************************/
    struct Button {
        float x{}, y{}, w{}, h{};               //!< Screen-space rectangle.
        std::string label;                      //!< Optional text label.
        std::function<void()> onClick;          //!< Callback fired after click feedback finishes.
        bool hovered{ false };                  //!< True while the cursor is inside the button.
        bool pressed{ false };                  //!< True while press feedback is displayed.
        unsigned idleTexture{ 0 };              //!< Texture used for the idle state.
        unsigned hoverTexture{ 0 };             //!< Texture used while hovered.
        bool useTextures{ false };              //!< True when the button should render textures.
        bool drawLabelOnTexture{ false };       //!< True to render text on top of the texture.
        std::function<void()> onHover;          //!< Callback fired when the cursor first enters.
        bool wasHovered = false;                //!< Previous-frame hover state for edge detection.
    };

    /*************************************************************************
      \brief  Remove all registered buttons and pending interaction state.
    *************************************************************************/
    void Clear();

    /*************************************************************************
      \brief  Add a text-only button.
      \param  x        Left position in screen-space pixels.
      \param  y        Bottom position in screen-space pixels.
      \param  w        Width in pixels.
      \param  h        Height in pixels.
      \param  label    Label drawn on the button.
      \param  onClick  Callback fired when the button is clicked.
    *************************************************************************/
    void AddButton(float x, float y, float w, float h,
        const std::string& label,
        std::function<void()> onClick);

    /*************************************************************************
      \brief  Add a texture-backed button.
      \param  x                  Left position in screen-space pixels.
      \param  y                  Bottom position in screen-space pixels.
      \param  w                  Width in pixels.
      \param  h                  Height in pixels.
      \param  label              Optional label to draw.
      \param  idleTexture        Texture shown in the idle state.
      \param  hoverTexture       Texture shown while hovered; falls back to idle when zero.
      \param  onClick            Callback fired when the button is clicked.
      \param  drawLabelOnTexture True to draw the label on top of the texture.
    *************************************************************************/
    void AddButton(float x, float y, float w, float h,
        const std::string& label,
        unsigned idleTexture,
        unsigned hoverTexture,
        std::function<void()> onClick,
        bool drawLabelOnTexture = false);

    /*************************************************************************
      \brief  Poll mouse input and update button hover/press state.
      \param  input  Reserved InputSystem pointer (currently unused).
    *************************************************************************/
    void Update(Framework::InputSystem* input);

    /*************************************************************************
      \brief  Draw every registered button.
      \param  render  Render system used for screen size and optional text drawing.
    *************************************************************************/
    void Draw(Framework::RenderSystem* render);

    /*************************************************************************
      \brief  Set the hover callback on the most recently added button.
      \param  onHover  Callback fired when the cursor enters that button.
    *************************************************************************/
    void SetLastHoverCallback(std::function<void()> onHover);

    /*************************************************************************
      \brief  Set a shared callback for all button select events.
      \param  onSelect  Callback typically used for click/select sounds.
    *************************************************************************/
    void SetSelectSoundCallback(std::function<void()> onSelect);

private:
    std::vector<Button> buttons_;               //!< Active button list for the current menu.
    bool prevMouseDown_{ false };               //!< Previous left-mouse state for edge detection.
    int activeButtonIndex_{ -1 };               //!< Button currently showing pressed feedback.
    double callbackDispatchTime_{ 0.0 };        //!< GLFW time when the pending callback should fire.
    bool callbackPending_{ false };             //!< True while a click callback is deferred.
    std::function<void()> pendingCallback_;     //!< Deferred click callback captured on press.
    std::function<void()> onSelectSound_;       //!< Shared callback fired on button selection.

    /*************************************************************************
      \brief  Return true when a point lies inside the button rectangle.
    *************************************************************************/
    static bool Contains(const Button& b, double mx, double my);

    /*************************************************************************
      \brief  Detect a rising-edge left click and update the cached previous state.
    *************************************************************************/
    static bool RisingEdgeLeftClick(bool now, bool& prev);
};
