#include "scannertwain.h"

ScannerTwain::ScannerTwain(HWND window_handle)
{
    hWnd = window_handle;
}

ScannerTwain::~ScannerTwain()
{

}

bool ScannerTwain::Scan(QString filename)
{
    bool ret = false;
    if(LoadDLL()){
        if(OpenSourceManager()){
            if(OpenSource()){
                TwainSetting();
                if(Enabled()){
                    if(TransferImage(filename)){
                        ret = true;
                    }
                    Disabled();
                }
                CloseSource();
            }
            CloseSourceManager();
        }
        UnLoadDLL();
    }
    return ret;
}

float ScannerTwain::Fix32ToFloat(TW_FIX32 fix32)
{
    float floater;
    floater = (float)fix32.Whole + (float)fix32.Frac / (float)65536.0;
    return floater;
}


TW_FIX32 ScannerTwain::FloatToFix32(float floater)
{
    TW_FIX32 Fix32_value;
    TW_INT32 value = (TW_INT32) (floater * 65536.0 + 0.5);
    Fix32_value.Whole =(short)( value >> 16);
    Fix32_value.Frac =(short)( value & 0x0000ffffL);
    return (Fix32_value);
}

bool ScannerTwain::LoadDLL()
{
    OFSTRUCT of;

    //QString dll_file_name = "C:\\Windows\\twain_32.dll";
    //QString dll_file_name = "twain_32.dll";   //WindowsにプリインストールされているDLLだが，32bit版しかない．
    QString dll_file_name = "TWAINDSM.dll"; //64bit版が，次のURLからダウンロードできる　https://sourceforge.net/projects/twain-dsm/
    LPCWSTR dll_file_name_wchar = (const wchar_t*) dll_file_name.utf16();

    // DLLファイルの確認
    if (OpenFile(dll_file_name.toUtf8().data(), &of, OF_EXIST) == -1){
        return FALSE;
    }

    // DLLのロード
    if ( (hDSMDLL = LoadLibrary(dll_file_name_wchar)) == NULL){
        return FALSE;
    }

    // 関数ポインタの取得
    if( (lpfnDSM_Entry = (DSMENTRYPROC)GetProcAddress(hDSMDLL,"DSM_Entry")) == NULL){
        UnLoadDLL();
        return FALSE;
    }

    qDebug ("Load dll complete");
    return TRUE;
}

bool ScannerTwain::UnLoadDLL()
{
    if(hDSMDLL)
        return FreeLibrary(hDSMDLL);

    return TRUE;
}

bool ScannerTwain::OpenSourceManager()
{
    TW_UINT16 rc;		// 戻値

    // アプリケーションＩＤの初期化
    pAppId.Id = 0;		// 0にする
    pAppId.Version.MajorNum = 2;	// アプリケーションのバージョンメジャー番号
    pAppId.Version.MinorNum = 0;	// アプリケーションのバージョンマイナー番号
    pAppId.Version.Language = TWLG_JAPANESE;// 言語
    pAppId.Version.Country = TWCY_JAPAN;	// 国
    StringCbCopy ((wchar_t *)pAppId.Version.Info, 34, L"2.0.10");	// バージョン文字列
    pAppId.Version.Info[0] = '0';
    pAppId.ProtocolMajor = TWON_PROTOCOLMAJOR;
    pAppId.ProtocolMinor = TWON_PROTOCOLMINOR;
    pAppId.SupportedGroups = DG_IMAGE | DG_CONTROL;
    StringCbCopy ((wchar_t *)pAppId.Manufacturer, 34, L"Nanshin Institute of Technology");	// アプリのメーカー
    StringCbCopy ((wchar_t *)pAppId.ProductFamily, 34, L"OKAMOTO LAB");	//アプリの製品ファミリー
    StringCbCopy ((wchar_t *)pAppId.ProductName, 34, L"Pic2NC");//アプリの製品名

    // ソースマネージャのオープン
    rc=lpfnDSM_Entry( &pAppId, NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, (TW_MEMREF)&hWnd );

    if(rc != TWRC_SUCCESS){
        qDebug("OpenSourceManager error");
        return FALSE;
    }

    qDebug("OpenSourceManager success");
    return TRUE;

}

bool ScannerTwain::CloseSourceManager()
{
    TW_UINT16 rc;		// 戻値
    rc=lpfnDSM_Entry( &pAppId, NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, (TW_MEMREF)&hWnd );

    if(rc != TWRC_SUCCESS)
        return FALSE;

    return TRUE;
}

bool ScannerTwain::OpenSource()
{
    TW_UINT16 rc;// 戻値
    pSourceId.ProductName[0]='\0';
    pSourceId.Id=0;

    //DS選択のダイアログ表示
    rc = lpfnDSM_Entry( &pAppId, NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &pSourceId );

    if(rc == TWRC_CANCEL){
        qDebug("OpenSource canceled");
        return FALSE;
    }

    rc = lpfnDSM_Entry( &pAppId, NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &pSourceId );

    if(rc != TWRC_SUCCESS){
        qDebug("OpenSource error");
        return FALSE;
    }

    qDebug("OpenSource success");
    return TRUE;
}

bool ScannerTwain::CloseSource()
{
    TW_UINT16 rc = lpfnDSM_Entry( &pAppId, NULL, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &pSourceId );

    if(rc != TWRC_SUCCESS)
        return FALSE;

    return TRUE;
}

bool ScannerTwain::Enabled()
{
    TW_UINT16 rc;// 戻値
    TW_USERINTERFACE ui;

    ui.ShowUI = FALSE;
    ui.ModalUI = FALSE;
    ui.hParent = hWnd;

    rc = lpfnDSM_Entry( &pAppId, &pSourceId, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, &ui );

    if(rc != TWRC_SUCCESS){
        qDebug("Enabled error");
        return FALSE;
    }

    qDebug("Enabled");
    return TRUE;
}

bool ScannerTwain::Disabled()
{
    TW_UINT16 rc;// 戻値
    TW_USERINTERFACE ui;

    ui.ShowUI = FALSE;
    ui.ModalUI = FALSE;
    ui.hParent = hWnd;

    rc = lpfnDSM_Entry( &pAppId, &pSourceId, DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &ui );

    if(rc != TWRC_SUCCESS){
        return FALSE;
    }

    return TRUE;
}

bool ScannerTwain::TransferImage(QString filename){
    LPBITMAPINFOHEADER	bi;		//BITMAPINFOHEADERのポインタ
    TW_PENDINGXFERS		PendingXfers;
    TW_UINT32	hBitmap;		//画像情報(BITMAPINFOHEADER + 画像データ)
    TW_UINT16	rc;				// 戻値

    // 画像転送
    bi		= NULL;
    hBitmap = NULL;
    rc = lpfnDSM_Entry( &pAppId, &pSourceId, DG_IMAGE, DAT_IMAGENATIVEXFER, MSG_GET, (TW_MEMREF)&hBitmap );
    //BITMAPINFOHEADERのポインタ取得
    bi = (LPBITMAPINFOHEADER)GlobalLock((void*)hBitmap);

    // 戻値のチェック
    PendingXfers.Count = -1;
    switch(rc){
        case TWRC_XFERDONE:	//成功
            if(bi != NULL){

                //画像を一旦ファイルに保存してから再度読み込む
                if (WriteImage(bi, filename) == FALSE){	//BMPファイル保存
                    qDebug("BMP Write Error");
                    lpfnDSM_Entry( &pAppId, &pSourceId, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &PendingXfers );	//終了処理
                    return FALSE;
                }
            }

            lpfnDSM_Entry( &pAppId, &pSourceId, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &PendingXfers );	//終了処理
            //cout << "残り" << PendingXfers.Count << "枚" << endl;
            break;

        case TWRC_CANCEL:	//キャンセル
            qDebug("キャンセルされました");
            lpfnDSM_Entry( &pAppId, &pSourceId, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &PendingXfers );	//リセット処理
            return FALSE;
            break;

        case TWRC_FAILURE:	//エラー
            qDebug("転送中にエラーが発生しました");
            lpfnDSM_Entry( &pAppId, &pSourceId, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &PendingXfers );	//リセット処理
            return FALSE;
            break;
    }

    if(bi != NULL){
        GlobalUnlock((void*)hBitmap);
        GlobalFree((void*)hBitmap);
    }

    return TRUE;
}

bool ScannerTwain::WriteImage(LPBITMAPINFOHEADER bi, QString filename){
    DWORD	hsize;
    FILE	*fp;
    BITMAPFILEHEADER	bf;

    //BMPのヘッダのサイズ
    hsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    if((fp = fopen(filename.toUtf8(), "w")) == NULL){
        return FALSE;
    }

    //BMPファイルのヘッダ情報
    bf.bfType		= *(WORD*)"BM";				//BM固定
    bf.bfSize		= hsize + bi->biSizeImage;	//BMPファイルのサイズ = ヘッダサイズ + 画像サイズ
    bf.bfReserved1	= 0;						//0固定
    bf.bfReserved2	= 0;						//0固定
    bf.bfOffBits	= hsize;					//画像読み込み時のオフセット = ヘッダのサイズ

    //BITMAPFILEHEADER書き込み
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, fp);
    //BITMAPINFOHEADER + 画像情報を一気に書き込み
    fwrite(bi, sizeof(BITMAPINFOHEADER) + bi->biSizeImage, 1, fp);

    fclose(fp);

    return TRUE;
}


bool ScannerTwain::TwainSetting(void){
    TW_IMAGELAYOUT  Image;
    TW_UINT16		rc;		// 戻値
    TW_CAPABILITY	twCapability;
    pTW_ONEVALUE	ptwOneValue;

    //コンテナを設定する
    twCapability.Cap		= ICAP_PIXELTYPE;	//設定内容をICAP_XXXで指定する
    twCapability.ConType	= TWON_ONEVALUE;	//コンテナのタイプを指定する

    //コンテナ確保
    twCapability.hContainer = GlobalAlloc(GHND,sizeof(TW_ONEVALUE));
    ptwOneValue = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);

    //アイテムを設定
    ptwOneValue->ItemType	= TWTY_UINT16;
    ptwOneValue->Item		= TWPT_RGB;		//24bitカラーで保存

    //コマンド
    rc = lpfnDSM_Entry( &pAppId, &pSourceId, DG_CONTROL, DAT_CAPABILITY, MSG_SET, (TW_MEMREF)&twCapability );

    //コンテナ開放
    GlobalUnlock(twCapability.hContainer);
    GlobalFree(twCapability.hContainer);

    if(rc != TWRC_SUCCESS){
        qDebug("Setting Error.");
        return FALSE;
    }

    //スキャン範囲設定 右上が原点
    memset(&Image, 0, sizeof(TW_IMAGELAYOUT));
    Image.Frame.Left     = FloatToFix32(0);
    Image.Frame.Right    = FloatToFix32(6);
    Image.Frame.Top      = FloatToFix32(0);
    Image.Frame.Bottom   = FloatToFix32(8);
    rc = lpfnDSM_Entry( &pAppId, &pSourceId, DG_IMAGE, DAT_IMAGELAYOUT, MSG_SET, (TW_MEMREF)&Image );

    return TRUE;
}
