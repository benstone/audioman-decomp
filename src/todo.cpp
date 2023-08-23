#include "todo.h"
#include "dpf.h"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <Windows.h>

void ShowNotImplementedMessage(const char *functionName)
{
    static bool ignore = false;
    char message[512];

    // Build the message
    sprintf_s(message,
              "This function is not yet implemented. Please raise a GitHub issue containing the following function "
              "name:\n\n%s\n",
              functionName);
    DPRINTF(0, message);

    if (ignore)
    {
        return;
    }

    // Show message box
    strcat_s(message, "\nChoose Abort to exit the program, Retry to continue or Ignore to silence these messages.");
    int result = MessageBoxA(NULL, message, "AudioMan", MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION);

    switch (result)
    {
    case IDABORT:
        ExitProcess(-1);
        break;
    case IDRETRY:
        // do nothing
        break;
    case IDIGNORE:
        ignore = true;
        break;
    default:
        assert(FALSE);
        break;
    }
}
