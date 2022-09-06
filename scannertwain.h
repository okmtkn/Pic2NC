#ifndef SCANNERTWAIN_H
#define SCANNERTWAIN_H

#include "twain.h"
#include <windows.h>
#include <strsafe.h>
#include <QString>


class ScannerTwain
{
public:
    ScannerTwain(HWND window_handle);
    ~ScannerTwain();

public:
    bool LoadDLL();
    bool UnLoadDLL();
    bool OpenSourceManager();
    bool CloseSourceManager();
    bool OpenSource();
    bool CloseSource();
    bool Enabled();
    bool Disabled();
    bool TransferImage();
    bool WriteImage(LPBITMAPINFOHEADER	bi);
    bool TwainSetting(void);

private:
    float    Fix32ToFloat (TW_FIX32 fix32);
    TW_FIX32 FloatToFix32 (float floater);

private:
    HWND         hWnd;
    DSMENTRYPROC lpfnDSM_Entry;
    HMODULE      hDSMDLL;
    TW_IDENTITY  pAppId;
    TW_IDENTITY  pSourceId;
};

#endif // SCANNERTWAIN_H
