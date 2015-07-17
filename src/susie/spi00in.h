#ifndef spi00in_h
#define spi00in_h

#include <windows.h>

/*-------------------------------------------------------------------------*/
/* 画像情報構造体 */
/*-------------------------------------------------------------------------*/
#pragma pack(push)
#pragma pack(1) //構造体のメンバ境界を1バイトにする
typedef struct PictureInfo {
	long left,top;		/* 画像を展開する位置 */
	long width;			/* 画像の幅(pixel) */
	long height;		/* 画像の高さ(pixel) */
	WORD x_density;		/* 画素の水平方向密度 */
	WORD y_density;		/* 画素の垂直方向密度 */
	short colorDepth;	/* 画素当たりのbit数 */
	HLOCAL hInfo;		/* 画像内のテキスト情報 */
} PictureInfo;
#pragma pack(pop)

/*-------------------------------------------------------------------------*/
/* エラーコード */
/*-------------------------------------------------------------------------*/
#define SPI_NO_FUNCTION			-1	/* その機能はインプリメントされていない */
#define SPI_ALL_RIGHT			0	/* 正常終了 */
#define SPI_ABORT				1	/* コールバック関数が非0を返したので展開を中止した */
#define SPI_NOT_SUPPORT			2	/* 未知のフォーマット */
#define SPI_OUT_OF_ORDER		3	/* データが壊れている */
#define SPI_NO_MEMORY			4	/* メモリーが確保出来ない */
#define SPI_MEMORY_ERROR		5	/* メモリーエラー */
#define SPI_FILE_READ_ERROR		6	/* ファイルリードエラー */
#define	SPI_WINDOW_ERROR		7	/* 窓が開けない (非公開のエラーコード) */
#define SPI_OTHER_ERROR			8	/* 内部エラー */
#define	SPI_FILE_WRITE_ERROR	9	/* 書き込みエラー (非公開のエラーコード) */
#define	SPI_END_OF_FILE			10	/* ファイル終端 (非公開のエラーコード) */

/*-------------------------------------------------------------------------*/
/* '00IN'関数のプロトタイプ宣言 */
/*-------------------------------------------------------------------------*/
typedef int (CALLBACK *SPI_PROGRESS)(int, int, long);
	int __declspec(dllexport) __stdcall GetPluginInfo
			(int infono, LPSTR buf, int buflen);
	int __declspec(dllexport) __stdcall IsSupported(LPSTR filename, DWORD dw);
	int __declspec(dllexport) __stdcall GetPictureInfo
			(LPSTR buf,long len, unsigned int flag, PictureInfo *lpInfo);
	int __declspec(dllexport) __stdcall GetPicture
			(LPSTR buf,long len, unsigned int flag,
			 HANDLE *pHBInfo, HANDLE *pHBm,
			 SPI_PROGRESS lpPrgressCallback, long lData);
	int __declspec(dllexport) __stdcall GetPreview
			(LPSTR buf,long len, unsigned int flag,
			 HANDLE *pHBInfo, HANDLE *pHBm,
			 SPI_PROGRESS lpPrgressCallback, long lData);

#endif
