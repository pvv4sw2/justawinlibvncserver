#include <rfb/rfb.h>

#include <WinSpecific.h>


#define BPP      4

/* 15 frames per second (if we can) */
#define PICTURE_TIMEOUT (1000.0/15.0)


/*
 * throttle camera updates
*/
int TimeToTakePicture() {
    DWORD dwNow = 0, dwThen = 0, elapsed = 0;

    dwNow = GetTickCount();
    elapsed = dwNow - dwThen;

    if (elapsed > PICTURE_TIMEOUT) 
        dwThen = dwNow;

    return elapsed > PICTURE_TIMEOUT;
}


double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
    LARGE_INTEGER li;

    if(!QueryPerformanceFrequency(&li))
        return;
        
    PCFreq = ((double)(li.QuadPart))/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}

double GetCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return ((double)(li.QuadPart-CounterStart))/PCFreq;
}


double prevTimeSShot = 0, currTimeSShot = 0;
double timeEvents = 0;
double prevTimeStats = 0, timeStats = 0;

int main(int argc,char** argv)
{                                       
  long usec;
  int port = -1;
  rfbScreenInfoPtr server = NULL;
  char* authData[2] = { NULL, NULL};
  
  int width =  -1;
  int height = -1;
  int left =  -1;
  int top = -1;

  BOOL ro = TRUE;


  if((argc != 7) && (argc != 8))
  {
      printf("Usage: WinLibVNCServer <left> <top> <width> <height> <port> <password> [/manage]"); 
      return -1;
  }

  left = atoi(argv[1]);
  if(!left && strcmp(argv[1], "0")) left = -1;
  top = atoi(argv[2]);
  if(!top && strcmp(argv[2], "0")) top = -1;
  width = (atoi(argv[3]) / 4)*4;
  height = atoi(argv[4]);
  port = atoi(argv[5]);
  authData[0] = argv[6];

  if((left < 0) || (top < 0) || (width <= 0) || (height <= 0) || (port <=0))
  {
      printf("Invalid command line parameter(s)"); 
      return -1;
  }  

  if((8 == argc) && (stricmp(argv[7], "/manage")))
  {
      printf("Invalid command line parameter(s)"); 
      return -1;
  } 
  else
  {
      ro = FALSE;
  }
    

  if(!winInitScreenCapture(left, top, width, height))
      return 0;


  server = rfbGetScreen(&argc, argv, width, height, 8, 3, BPP);
  if(!server)
    return 0;

  server->port = port;
  server->passwordCheck = rfbCheckPasswordByList;
  server->authPasswdData = authData;

  server->desktopName = "WinLibVNCServer";
  server->frameBuffer = (char*)malloc(width*height*BPP);
  server->alwaysShared = (1==1);

  if(!ro)
  {
      winInitPointerHandler(rfbDefaultPtrAddEvent, left, top);
      server->ptrAddEvent = winPtrAddEvent;
  }

  /* Initialize the server */
  rfbInitServer(server);           

  StartCounter();
  /* Loop, processing clients and taking pictures */
  while (rfbIsActive(server)) {
    int picturetaken = 0;
    if (picturetaken = TimeToTakePicture())
    {
      prevTimeSShot = GetCounter();
      if (winTakePicture32((unsigned char *)server->frameBuffer))
        rfbMarkRectAsModified(server, 0, 0, width, height);
        
        prevTimeStats = currTimeSShot = GetCounter();
        
        fprintf(stderr,"Screen capture: %5.2f ms,"
                       " events: %5.2f ms,"
                       " printf(): %5.2f ms\r", 
                        currTimeSShot - prevTimeSShot,
                        timeEvents, timeStats);
        timeStats = GetCounter() - prevTimeStats;
    }
      
    if(picturetaken)
        timeEvents = GetCounter();
    usec = server->deferUpdateTime*1000;
    rfbProcessEvents(server,usec);
    if(picturetaken) timeEvents = GetCounter() - timeEvents;
  }

  winShutdownScreenCapture();
  return(0);
}
