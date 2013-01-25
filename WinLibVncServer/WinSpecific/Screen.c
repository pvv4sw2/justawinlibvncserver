#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>

#include "WinSpecific.h"


static int width_ =  -1;
static int height_ = -1;
static int left_ =  -1;
static int top_ = -1;

HWND hDesktopWnd_ = NULL;
HDC hDesktopDC_ = NULL;
HDC hCaptureDC_ = NULL;


int winTakePicture32(unsigned char *buffer)
{
  struct 
  {
    BITMAPINFO BitmapInfo;
    char colors[255 * sizeof(RGBQUAD)];
  } biBuf;    

  HBITMAP hCaptureBitmap = 
                CreateCompatibleBitmap( hDesktopDC_, width_, height_); 
  memset(&biBuf, 0, sizeof(biBuf));
  SelectObject(hCaptureDC_,hCaptureBitmap); 
  BitBlt(hCaptureDC_, 0, 0, width_, height_, hDesktopDC_, left_, top_, SRCCOPY|CAPTUREBLT);

  biBuf.BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  if (0 != 
       GetDIBits(hCaptureDC_, hCaptureBitmap, 0, 0, NULL, &(biBuf.BitmapInfo), DIB_RGB_COLORS))
  {
    BITMAP Bitmap;
    DWORD dwBmpSize = 0;
    biBuf.BitmapInfo.bmiHeader.biHeight *= -1;

    GetObject(hCaptureBitmap,sizeof(BITMAP),&Bitmap);
    dwBmpSize = 
        ((Bitmap.bmWidth * biBuf.BitmapInfo.bmiHeader.biBitCount + 31) / 32) * 4 * Bitmap.bmHeight;
    GetDIBits(hCaptureDC_, hCaptureBitmap, 0, height_, buffer, &(biBuf.BitmapInfo), DIB_RGB_COLORS);

    {
      DWORD i;
      for(i = 0; i < dwBmpSize; i += 4)
      {
        char temp = buffer[i];
        buffer[i] = buffer[i + 2];
        buffer[i + 2] = temp;
      }
    }
  }
            
  DeleteObject(hCaptureBitmap);

  return (1==1);
}

int winInitScreenCapture(int left, int top, int width, int height)
{
  left_ = left; top_ = top; width_ = width; height_ = height;

  if(hDesktopWnd_ = GetDesktopWindow())
  {    
    if(hDesktopDC_ = CreateDCA("DISPLAY", NULL, NULL, NULL))
    {
      hCaptureDC_ = CreateCompatibleDC(hDesktopDC_);
    }
  };

  if(hCaptureDC_ && hDesktopDC_ && hDesktopWnd_) return (1 == 1);

  winShutdownScreenCapture();
  return (1 != 1);
}

void winShutdownScreenCapture()
{
  if(hCaptureDC_) DeleteDC(hCaptureDC_);
  if(hDesktopDC_) DeleteDC(hDesktopDC_);
  
  hCaptureDC_ = NULL;
  hDesktopDC_ = NULL;
};