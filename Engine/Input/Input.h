#pragma once

#include <unordered_map>
struct GLFWwindow;

namespace Framework
{
	struct MouseState
	{
		double x = 0;
		double y = 0;
		bool leftClick = false;
		bool rightClick = false;
	};

	class InputManager
	{
	public:
		InputManager(GLFWwindow* window);
		~InputManager() = default;

		void Update();

		// Keyboard queries
		bool IsKeyPressed(int key) const;
		bool IsKeyHeld(int key) const;
		bool IsKeyReleased(int key) const;

		// Mouse Queries
		MouseState GetMouseState() const;
		bool IsMousePressed(int button) const;
		bool IsMouseHeld(int button) const;
		bool IsMouseReleased(int button) const;

	private:
		GLFWwindow* m_window;

		std::unordered_map<int, bool> m_keyHeld;
		std::unordered_map<int, bool> m_keyPressed;
		std::unordered_map<int, bool> m_keyReleased;

		std::unordered_map<int, bool> m_mouseHeld;
		std::unordered_map<int, bool> m_mousePressed;
		std::unordered_map<int, bool> m_mouseReleased;

		MouseState m_mouseState;
	};
}
