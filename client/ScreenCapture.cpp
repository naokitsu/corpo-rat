#include "ScreenCapture.h"

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
  using namespace Gdiplus;
  UINT  num = 0;
  UINT  size = 0;

  ImageCodecInfo* pImageCodecInfo = nullptr;

  GetImageEncodersSize(&num, &size);
  if(size == 0)
    return -1;

  pImageCodecInfo = static_cast<ImageCodecInfo *>(malloc(size));
  if(pImageCodecInfo == nullptr)
    return -1;

  GetImageEncoders(num, size, pImageCodecInfo);
  for(UINT j = 0; j < num; ++j)
  {
    if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
    {
      *pClsid = pImageCodecInfo[j].Clsid;
      free(pImageCodecInfo);
      return j;
    }    
  }
  free(pImageCodecInfo);
  return 0;
}

IStream *Screenshot() {
  using namespace Gdiplus;
  IStream* istream;
  HRESULT res = CreateStreamOnHGlobal(nullptr, true, &istream);
  const GdiplusStartupInput gdiplus_startup_input;
  ULONG_PTR gdiplus_token;
  GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, nullptr);
  {
    const int height = GetSystemMetrics(SM_CYSCREEN);
    const int width = GetSystemMetrics(SM_CXSCREEN);
		
    const HDC screen_dc = ::GetDC(nullptr);
    const HDC comp_dc = CreateCompatibleDC(screen_dc);
    const HBITMAP bitmap_handle = CreateCompatibleBitmap(screen_dc, width, height);
    auto old_bitmap = static_cast<HBITMAP>(SelectObject(comp_dc, bitmap_handle));
    BitBlt(comp_dc, 0, 0, width, height, screen_dc, 0, 0, SRCCOPY);
		
    Gdiplus::Bitmap bitmap(bitmap_handle, nullptr);
    CLSID clsid;
    GetEncoderClsid(L"image/jpeg", &clsid);
    bitmap.Save(istream, &clsid, nullptr);
		
    DeleteObject(comp_dc);
    DeleteObject(bitmap_handle);
    ReleaseDC(nullptr,screen_dc);
  }
  GdiplusShutdown(gdiplus_token);
  istream->Seek(LARGE_INTEGER {0}, STREAM_SEEK_SET, nullptr);
  return istream;
}

