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
    bool Scan(QString filename);

private:
    bool LoadDLL();
    bool UnLoadDLL();
    bool OpenSourceManager();
    bool CloseSourceManager();
    bool OpenSource();
    bool CloseSource();
    bool Enabled();
    bool Disabled();
    bool TransferImage(QString filename);
    bool WriteImage(LPBITMAPINFOHEADER	bi, QString filename);
    bool TwainSetting(void);
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
