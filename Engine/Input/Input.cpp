//Input.cpp
#include "Input.h"
#include <GLFW/glfw3.h>
#include "Windows.h"
#include <iostream>


namespace Framework
{
	InputManager::InputManager(GLFWwindow* window) : m_window(window)
	{
		// Initialize keys of interest
		int keys[] = {
			GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D,
			GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT
		};

		for (int key : keys)
		{
			m_keyHeld[key] = false;
		}

		// Initialize the mouse buttons
		int buttons[] = { GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT };
		for (int btn : buttons)
		{
			m_mouseHeld[btn] = false;
		}
	}

	void InputManager::Update()
	{
		m_keyPressed.clear();
		m_keyReleased.clear();
		m_mousePressed.clear();
		m_mouseReleased.clear();

		// Keyboard
		for (auto& [key, held] : m_keyHeld)
		{
			int state = glfwGetKey(m_window, key);
			if (state == GLFW_PRESS)
			{
				if (!held)
					m_keyPressed[key] = true;
				held = true;
			}
			else
			{
				if (held)
					m_keyReleased[key] = true;
				held = false;
			}
		}

		// Mouse buttons
		for (auto& [btn, held] : m_mouseHeld)
		{
			int state = glfwGetMouseButton(m_window, btn);
			if (state == GLFW_PRESS)
			{
				if (!held)
					m_mousePressed[btn] = true;
				held = true;
			}
			else
			{
				if (held)
					m_mouseReleased[btn] = true;
				held = false;
			}
		}

		// Mouse position
		glfwGetCursorPos(m_window, &m_mouseState.x, &m_mouseState.y);
		m_mouseState.leftClick = m_mouseHeld[GLFW_MOUSE_BUTTON_LEFT];
		m_mouseState.rightClick = m_mouseHeld[GLFW_MOUSE_BUTTON_RIGHT];
	}

	// Keyboard queries
	bool InputManager::IsKeyPressed(int key) const
	{
		auto it = m_keyPressed.find(key);
		return it != m_keyPressed.end() && it->second;
	}

	bool InputManager::IsKeyHeld(int key) const
	{
		auto it = m_keyHeld.find(key);
		return it != m_keyHeld.end() && it->second;
	}

	bool InputManager::IsKeyReleased(int key) const
	{
		auto it = m_keyReleased.find(key);
		return it != m_keyReleased.end() && it->second;
	}

	// Mouse queries
	MouseState InputManager::GetMouseState() const
	{
		return m_mouseState;
	}

	bool InputManager::IsMousePressed(int button) const
	{
		auto it = m_mousePressed.find(button);
		return it != m_mousePressed.end() && it->second;
	}

	bool InputManager::IsMouseHeld(int button) const
	{
		auto it = m_mouseHeld.find(button);
		return it != m_mouseHeld.end() && it->second;
	}

	bool InputManager::IsMouseReleased(int button) const
	{
		auto it = m_mouseReleased.find(button);
		return it != m_mouseReleased.end() && it->second;
	}

	bool IsDown(int vk)
	{
		// High-order bit set means the key is currently down.
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}
}
