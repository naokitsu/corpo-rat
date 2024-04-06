#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <iostream>

inline int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
IStream *Screenshot();