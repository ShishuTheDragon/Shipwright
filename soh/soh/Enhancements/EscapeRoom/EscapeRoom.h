#include "libultraship/libultra/types.h"

class CustomMessage;

extern "C" {
    void EscapeRoom_RegisterHooks();
    CustomMessage EscapeRoom_GetCustomMessage(u16 textId);
}
