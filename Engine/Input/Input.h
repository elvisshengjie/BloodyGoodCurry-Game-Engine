#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>


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

	// Returns true while the given virtual-key is held down.
// vk accepts Windows virtual key codes (e.g., 'Q', 'E', 'Z', 'X', 'R', VK_SHIFT, etc.).
	bool IsDown(int vk);

}
