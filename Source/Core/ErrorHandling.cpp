#include "ErrorHandling.h"

#include <SDL_messagebox.h>
#include <string>

namespace An
{

	// ***********************************************************************

	void Assertion(const char* errorMsg, const char* file, int line)
	{
		switch (ShowAssertDialog(errorMsg, file, line))
		{
		case 0:
			_set_abort_behavior(0, _WRITE_ABORT_MSG);
			abort();
			break;
		case 1:
			__debugbreak();
			break;
		default:
			break;
		}
		return;
	}

	// ***********************************************************************

	int ShowAssertDialog(const char * errorMsg, const char * file, int line)
	{
		const SDL_MessageBoxButtonData buttons[] = {
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Abort" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Debug" },
		{ 0, 2, "Continue" },
		};

		std::string message;
		// Replace with c++20 format?
		// message.sprintf("Assertion Failed\n\n%s\n\nFile: %s\nLine %i", errorMsg, file, line);

		//Log::Crit("%s", message.c_str());

		const SDL_MessageBoxData messageboxdata = {
			SDL_MESSAGEBOX_ERROR,
			NULL,
			"Error",
			message.c_str(),
			SDL_arraysize(buttons),
			buttons,
			nullptr
		};
		int buttonid;
		SDL_ShowMessageBox(&messageboxdata, &buttonid);
		return buttonid;
	}

}