#pragma once

#include <rfb/rfb.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  winInitScreenCapture(int left, int top, int width, int height);
extern void winShutdownScreenCapture();
extern int  winTakePicture32(unsigned char *buffer);

extern int winInitPointerHandler(rfbPtrAddEventProcPtr oldHandler, int left, int top);
extern void winPtrAddEvent (int buttonMask, int x, int y, struct _rfbClientRec* cl);

#ifdef __cplusplus
}
#endif