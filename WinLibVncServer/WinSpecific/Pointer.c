#include "WinSpecific.h"

static rfbPtrAddEventProcPtr oldPtrAddEventHandler = NULL;
static int scrleft = 0, scrtop = 0;

int winInitPointerHandler(rfbPtrAddEventProcPtr oldHandler, int left, int top)
{
  scrleft = left;
  scrtop = top;
  oldPtrAddEventHandler = oldHandler;
};

void winPtrAddEvent (int buttonMask, int x, int y, struct _rfbClientRec* cl)
{
  int rawx = (scrleft + x)*65535/GetSystemMetrics(SM_CXSCREEN);
  int rawy = (scrtop + y)*65535/GetSystemMetrics(SM_CYSCREEN);
  INPUT input;

  memset(&input, 0, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.dx = rawx;
  input.mi.dy = rawy;
  input.mi.mouseData = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

  SendInput(1, &input, sizeof(input));

  if(oldPtrAddEventHandler) oldPtrAddEventHandler(buttonMask, x, y, cl);
};
