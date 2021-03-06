#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <map>

#include "IncludeDeps.hpp"
#include "Core/Delegate.tpp"
#include "Core/KeyCode.hpp"

#include GLFW_INCLUDE
#include GLM_INCLUDE

namespace LWGC
{
	enum class ButtonAction {Press = GLFW_PRESS, Release = GLFW_RELEASE, Repeat = GLFW_REPEAT};
	enum class MouseMoveAction {Entered = GLFW_TRUE, Exited = GLFW_FALSE, Move = 2};

	using QuitDelegateIndex = DelegateIndex< void(void) >;
	using FocusDelegateIndex = DelegateIndex< void(int) >;
	using KeyDelegateIndex = DelegateIndex<void (LWGC::KeyCode, LWGC::ButtonAction, int)>;
	using MouseClickDelegateIndex = DelegateIndex<void (glm::vec<2, float, glm::qualifier::packed_highp>, LWGC::ButtonAction)>;
	using ScrollDelegateIndex = DelegateIndex< void(double, double) >;
	using MouseMoveDelegateIndex = DelegateIndex<void (glm::vec<2, float, glm::qualifier::packed_highp>, LWGC::MouseMoveAction)>;
	using CharDelegateIndex = DelegateIndex< void(uint32_t) >;

	class		EventSystem
	{
		private:
			GLFWwindow *	_window;
			glm::vec2		_mousePosition;
			glm::vec2		_oldMousePosition;
			int				_windowWidth;
			int				_windowHeight;

			static std::map< GLFWwindow *, EventSystem * > eventSystems;
			static EventSystem *	eventSystemInstance;

			void			UpdateMousePosition(void);

		public:
			Delegate< void(void) >								onQuit;
			Delegate< void(int) >								onFocus;
			Delegate< void(KeyCode, ButtonAction, int) >		onKey;
			Delegate< void(glm::vec2, int, ButtonAction) >		onMouseClick;
			Delegate< void(double, double) >					onScroll;
			Delegate< void(glm::vec2, MouseMoveAction) >		onMouseMove;
			Delegate< void(uint32_t) >							onChar;
			glm::vec2											delta;

			EventSystem(void);
			EventSystem(const EventSystem&) = delete;
			virtual ~EventSystem(void);

			EventSystem &	operator=(EventSystem const & src) = delete;

			void			BindWindow(GLFWwindow *window);

			void			LockCursor(void);
			void			ReleaseCursor(void);
			void			ToggleLockCursor(void);
			bool			IsCursorLocked(void);

			const glm::vec2			GetNormalizedCursorPosition(void) const;

			static EventSystem *	Get(void);
	};

	std::ostream &	operator<<(std::ostream & o, EventSystem const & r);
}
