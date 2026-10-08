#pragma once

namespace Hooks
{
	// Resolves the two hooked game functions through the Address Library and detours them.
	// Installs nothing and returns false if either one can't be found or verified.
	bool Install();
}
