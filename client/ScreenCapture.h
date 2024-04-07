#pragma once

#include <windows.h>

inline int GetEncoderClsid(const WCHAR *format, CLSID *pClsid);
IStream *Screenshot();
