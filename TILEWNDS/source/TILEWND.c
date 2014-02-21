/* TILEWND.c - "TileWindow" free-shape window library, public release 1.1.3 (build 13)
     Copyright (c) 1999-2002 Takayuki 'January June' Suwa

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA. */

#pragma strings(readonly)
#include <stdlib.h>
#include <string.h>
#define INCL_BASE
#define INCL_ERRORS
#define INCL_PM
#include <os2.h>
#include "TILEWND.H"

/* some undocumented window message informations from Eugene "Validator" Evstigneev / Sinergetix software */
#define WM_MOVEALONGWINDOW  0x041e  /* sent when mouse pointer moved along window */
#define WM_MOVEACROSSWINDOW 0x041f  /* sent when mouse pointer moved across windows */

/*  */
#define MAXTILES 8192
#define RGB2LCOLOR(pxRGB) \
    (*((PLONG)(PRGB2)(pxRGB)) & (LONG)0x00ffffff)
#define MAXSTRIPS 2048
#define SMWP_COUNT 256

/*  */
const struct __TileWindowVersionInfo_t __TileWindowVersionInfo =
{
    {(UCHAR)1, (UCHAR)1, (UCHAR)3},  /* {1, 1, 3} in this release */
    (ULONG)13,                       /* build 13 */
};

/*  */
static const DEVOPENSTRUC xDeviceOpenStructure = {(PSZ)NULL,
                                                  (PSZ)"DISPLAY",
                                                  (PDRIVDATA)NULL,
                                                  (PSZ)NULL,
                                                  (PSZ)NULL,
                                                  (PSZ)NULL,
                                                  (PSZ)NULL,
                                                  (PSZ)NULL,
                                                  (PSZ)NULL};

/*  */
static void* _malloc(size_t size)
{
    PVOID pvMemory = (PVOID)NULL;
    (VOID)DosAllocMem(&pvMemory,
                      (ULONG)size,
                      PAG_READ | PAG_WRITE | PAG_COMMIT);
    return pvMemory;
}

/*  */
#define _free(ptr) \
  (VOID)DosFreeMem((PVOID)(ptr))

/*  */
static APIRET DosReadComplete(HFILE hf,
                              PVOID pv,
                              ULONG ul,
                              PULONG pul)
{
    APIRET rc;
    ULONG ulTotal, ulPart;
    for(ulTotal = (ULONG)0;
        ul != (ULONG)0;
        pv = (PVOID)&((PUCHAR)pv)[ulPart], ul -= ulPart, ulTotal += ulPart)
    {
        rc = DosRead(hf,
                     pv,
                     ul,
                     &ulPart);
        if(rc != NO_ERROR)
            return rc;
        if(ulPart == (ULONG)0)
            break;
    }
    *pul = ulTotal;
    return NO_ERROR;
}

/*  */
static VOID _SetMultWindowPos(PSWP pswp,
                              ULONG cswp)
{
    HAB hAB;
    ULONG ulCount;
    hAB = WinQueryAnchorBlock(HWND_DESKTOP);
    ulCount = cswp / SMWP_COUNT;
    while(ulCount != (ULONG)0)
    {
        (VOID)WinSetMultWindowPos(hAB,
                                  pswp,
                                  SMWP_COUNT);
        pswp = &pswp[SMWP_COUNT];
        ulCount--;
    }
    (VOID)WinSetMultWindowPos(hAB,
                              pswp,
                              cswp % SMWP_COUNT);
}

/*  */
HRGN GpiCreateRegionFromMaskBitmap(HPS hpsRegion,
                                   HPS hpsMask,
                                   PRECTL pxrectMask,
                                   LONG lMaskColorRGB,
                                   ULONG ulMaxRects)
{
    HBITMAP hbmp;
    BITMAPINFOHEADER2 xBitmapInfoHeader;
    RECTL xrectMaskPS;
    PRECTL pxrectTiles;
    RGB* prgbPels;
    ULONG ulTileCount;
    LONG lY;
    LONG lX;
    RECTL xrectWork;
    ULONG ulIndex;
    HRGN hrgn;
    hbmp = GpiSetBitmap(hpsMask,
                        (HBITMAP)NULLHANDLE);
    if(hbmp == (HBITMAP)NULLHANDLE)
        return (HRGN)RGN_ERROR;
    (VOID)GpiSetBitmap(hpsMask,
                       hbmp);
    xBitmapInfoHeader.cbFix = (ULONG)16;
    (VOID)GpiQueryBitmapInfoHeader(hbmp,
                                   &xBitmapInfoHeader);
    if(pxrectMask == (PRECTL)NULL)
    {
        xrectMaskPS.xLeft = (LONG)0;
        xrectMaskPS.yBottom = (LONG)0;
        xrectMaskPS.xRight = (LONG)xBitmapInfoHeader.cx;
        xrectMaskPS.yTop = (LONG)xBitmapInfoHeader.cy;
    }
    else
        xrectMaskPS = *pxrectMask;
    if(ulMaxRects == (ULONG)0)
        ulMaxRects = MAXTILES;
    pxrectTiles = (PRECTL)_malloc(sizeof(RECTL) * (size_t)ulMaxRects);
    prgbPels = (RGB*)_malloc(((size_t)xBitmapInfoHeader.cx * (size_t)24 + (size_t)31) / (size_t)32 * (size_t)1 * (size_t)4);
    ulTileCount = (ULONG)0;
    *(PULONG)&xBitmapInfoHeader.cPlanes = (ULONG)1 + ((ULONG)24 << 16);
    for(lY = xrectMaskPS.yBottom;
        lY < xrectMaskPS.yTop;
        lY++)
    {
        if(GpiQueryBitmapBits(hpsMask,
                              lY,
                              (LONG)1,
                              (PBYTE)prgbPels,
                              (PBITMAPINFO2)&xBitmapInfoHeader) == (LONG)GPI_ALTERROR)
        {
            _free((void*)prgbPels);
            _free((void*)pxrectTiles);
            return (HRGN)RGN_ERROR;
        }
        if(lY == xrectMaskPS.yBottom)
            switch(lMaskColorRGB)
            {
                case MASKCOLOR_LEFTBOTTOM:
                    lMaskColorRGB = RGB2LCOLOR(&prgbPels[xrectMaskPS.xLeft]);
                    break;
                case MASKCOLOR_RIGHTBOTTOM:
                    lMaskColorRGB = RGB2LCOLOR(&prgbPels[xrectMaskPS.xRight - (LONG)1]);
                    break;
            }
        for(lX = xrectMaskPS.xLeft;
            lX < xrectMaskPS.xRight;
            lX++)
            if(RGB2LCOLOR(&prgbPels[lX]) != lMaskColorRGB)
            {
                xrectWork.xLeft = lX - xrectMaskPS.xLeft;
                xrectWork.yBottom = lY - xrectMaskPS.yBottom;
                xrectWork.yTop = lY - xrectMaskPS.yBottom + (LONG)1;
                do
                    lX++;
                while(lX < xrectMaskPS.xRight &&
                      RGB2LCOLOR(&prgbPels[lX]) != lMaskColorRGB);
                xrectWork.xRight = lX-- - xrectMaskPS.xLeft;
                for(ulIndex = ulTileCount;
                    ulIndex != (ULONG)0;
                    ulIndex--)
                    if(pxrectTiles[ulIndex - (ULONG)1].xLeft == xrectWork.xLeft &&
                       pxrectTiles[ulIndex - (ULONG)1].xRight == xrectWork.xRight &&
                       pxrectTiles[ulIndex - (ULONG)1].yTop == xrectWork.yBottom)
                    {
                        pxrectTiles[ulIndex - (ULONG)1].yTop = xrectWork.yTop;
                        break;
                    }
                if(ulIndex == (ULONG)0 &&
                   ulTileCount < ulMaxRects)
                    pxrectTiles[ulTileCount++] = xrectWork;
            }
    }
    _free((void*)prgbPels);
    hrgn = GpiCreateRegion(hpsRegion,
                           (LONG)ulTileCount,
                           pxrectTiles);
    _free((void*)pxrectTiles);
    return hrgn;
}

/*  */
HPS GpiCreatePSMDCB(PBITMAPINFOHEADER2 pxBitmapInfoHeader)
{
    SIZEL xsz;
    HAB hab;
    HDC hdc;
    HPS hps;
    HBITMAP hbmp;
    if(pxBitmapInfoHeader->cbFix >= (ULONG)sizeof(BITMAPINFOHEADER))
    {
        if(pxBitmapInfoHeader->cbFix == (ULONG)sizeof(BITMAPINFOHEADER))
        {
            xsz.cx = (LONG)((PBITMAPINFOHEADER)pxBitmapInfoHeader)->cx;
            xsz.cy = (LONG)((PBITMAPINFOHEADER)pxBitmapInfoHeader)->cy;
        }
        else
        {
            xsz.cx = (LONG)pxBitmapInfoHeader->cx;
            xsz.cy = (LONG)pxBitmapInfoHeader->cy;
        }
        hab = WinQueryAnchorBlock(HWND_DESKTOP);
        hdc = DevOpenDC(hab,
                        OD_MEMORY,
                        (PSZ)"*",
                        (LONG)5,
                        (PDEVOPENDATA)&xDeviceOpenStructure,
                        (HDC)NULLHANDLE);
        if(hdc != (HDC)DEV_ERROR)
        {
            hps = GpiCreatePS(hab,
                              hdc,
                              &xsz,
                              PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
            if(hps != (HPS)GPI_ERROR)
            {
                hbmp = GpiCreateBitmap(hps,
                                       pxBitmapInfoHeader,
                                       (ULONG)0,
                                       (PBYTE)NULL,
                                       (PBITMAPINFO2)NULL);
                if(hbmp != (HBITMAP)GPI_ERROR)
                {
                    (VOID)GpiSetBitmap(hps,
                                       hbmp);
                    return hps;
                }
                (VOID)GpiDestroyPS(hps);
            }
            (VOID)DevCloseDC(hdc);
        }
    }
    return (HPS)GPI_ERROR;
}

/*  */
HPS GpiCreatePSMDCBFromMemory(PBITMAPINFOHEADER2 pxBitmapInfoHeader,
                              PVOID pvMemory,
                              PBITMAPINFO2 pxMemoryBitmapInfo)
{
    SIZEL xsz;
    HAB hab;
    HDC hdc;
    HPS hps;
    HBITMAP hbmp;
    if(pxBitmapInfoHeader->cbFix >= (ULONG)sizeof(BITMAPINFOHEADER))
    {
        if(pxBitmapInfoHeader->cbFix == (ULONG)sizeof(BITMAPINFOHEADER))
        {
            xsz.cx = (LONG)((PBITMAPINFOHEADER)pxBitmapInfoHeader)->cx;
            xsz.cy = (LONG)((PBITMAPINFOHEADER)pxBitmapInfoHeader)->cy;
        }
        else
        {
            xsz.cx = (LONG)pxBitmapInfoHeader->cx;
            xsz.cy = (LONG)pxBitmapInfoHeader->cy;
        }
        hab = WinQueryAnchorBlock(HWND_DESKTOP);
        hdc = DevOpenDC(hab,
                        OD_MEMORY,
                        (PSZ)"*",
                        (LONG)5,
                        (PDEVOPENDATA)&xDeviceOpenStructure,
                        (HDC)NULLHANDLE);
        if(hdc != (HDC)DEV_ERROR)
        {
            hps = GpiCreatePS(hab,
                              hdc,
                              &xsz,
                              PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
            if(hps != (HPS)GPI_ERROR)
            {
                hbmp = GpiCreateBitmap(hps,
                                       pxBitmapInfoHeader,
                                       CBM_INIT,
                                       (PBYTE)pvMemory,
                                       pxMemoryBitmapInfo);
                if(hbmp != (HBITMAP)GPI_ERROR)
                {
                    (VOID)GpiSetBitmap(hps,
                                       hbmp);
                    return hps;
                }
                (VOID)GpiDestroyPS(hps);
            }
            (VOID)DevCloseDC(hdc);
        }
    }
    return (HPS)GPI_ERROR;
}

/*  */
HPS GpiCreatePSMDCBFromResource(HMODULE hmodResource,
                                ULONG ulResourceID,
                                PSIZEL pxszBitmapCreated)
{
    HAB hab;
    HDC hdc;
    HPS hps;
    static const SIZEL xsz1 = {(LONG)1, (LONG)1};
    HBITMAP hbmp;
    BITMAPINFOHEADER2 xBitmapInfoHeader;
    hab = WinQueryAnchorBlock(HWND_DESKTOP);
    hdc = DevOpenDC(hab,
                    OD_MEMORY,
                    (PSZ)"*",
                    (LONG)5,
                    (PDEVOPENDATA)&xDeviceOpenStructure,
                    (HDC)NULLHANDLE);
    if(hdc != (HDC)DEV_ERROR)
    {
        hps = GpiCreatePS(hab,
                          hdc,
                          (PSIZEL)&xsz1,
                          PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
        if(hps != (HPS)GPI_ERROR)
        {
            hbmp = GpiLoadBitmap(hps,
                                 hmodResource,
                                 ulResourceID,
                                 (LONG)0,
                                 (LONG)0);
            if(hbmp != (HBITMAP)GPI_ERROR)
            {
                xBitmapInfoHeader.cbFix = (ULONG)16;
                (VOID)GpiQueryBitmapInfoHeader(hbmp,
                                               &xBitmapInfoHeader);
                pxszBitmapCreated->cx = (LONG)xBitmapInfoHeader.cx;
                pxszBitmapCreated->cy = (LONG)xBitmapInfoHeader.cy;
                (VOID)GpiSetPS(hps,
                               pxszBitmapCreated,
                               PU_PELS | GPIF_DEFAULT);
                (VOID)GpiSetBitmap(hps,
                                   hbmp);
                return hps;
            }
            (VOID)GpiDestroyPS(hps);
        }
        (VOID)DevCloseDC(hdc);
    }
    return (HPS)GPI_ERROR;
}

/*  */
static HPS GpiCreatePSMDCBFromHFile(HFILE hfBitmapFile,
                                    PSIZEL pxszBitmapCreated)
{
#pragma pack(1)
    struct
    {
        USHORT usType;
        ULONG cbSize;
        SHORT xHotspot;
        SHORT yHotspot;
        ULONG offBits;
    } xBitmapFileHeader_head;
#pragma pack()
    ULONG ulActual;
    union BitmapFileHeader_t
    {
        BITMAPINFOHEADER xOld;
        BITMAPINFOHEADER2 xNew;
    }* pxBitmapFileHeader_tail;
    BITMAPINFOHEADER2 xBitmapInfoHeader;
    ULONG ulBitmapDataSize;
    PVOID pvBitmapData;
    HPS hps;
    if(DosReadComplete(hfBitmapFile,
                       (PVOID)&xBitmapFileHeader_head,
                       (ULONG)sizeof(xBitmapFileHeader_head),
                       &ulActual) == NO_ERROR &&
       ulActual == (ULONG)sizeof(xBitmapFileHeader_head) &&
       xBitmapFileHeader_head.usType == BFT_BMAP &&
       (xBitmapFileHeader_head.cbSize == (ULONG)sizeof(BITMAPFILEHEADER) ||
        xBitmapFileHeader_head.cbSize >= (ULONG)sizeof(BITMAPFILEHEADER2) - (ULONG)sizeof(BITMAPINFOHEADER2) + (ULONG)16))
    {
        pxBitmapFileHeader_tail = (union BitmapFileHeader_t*)_malloc((size_t)xBitmapFileHeader_head.offBits - sizeof(xBitmapFileHeader_head));
        if(DosReadComplete(hfBitmapFile,
                           pxBitmapFileHeader_tail,
                           xBitmapFileHeader_head.offBits - (ULONG)sizeof(xBitmapFileHeader_head),
                           &ulActual) == NO_ERROR &&
           ulActual == xBitmapFileHeader_head.offBits - (ULONG)sizeof(xBitmapFileHeader_head))
        {
            xBitmapInfoHeader.cbFix = (ULONG)16;
            if(pxBitmapFileHeader_tail->xOld.cbFix == (ULONG)sizeof(BITMAPINFOHEADER))
            {
                xBitmapInfoHeader.cx = (ULONG)pxBitmapFileHeader_tail->xOld.cx;
                xBitmapInfoHeader.cy = (ULONG)pxBitmapFileHeader_tail->xOld.cy;
                *(PULONG)&xBitmapInfoHeader.cPlanes = *(PULONG)&pxBitmapFileHeader_tail->xOld.cPlanes;
            }
            else
            {
                xBitmapInfoHeader.cx = pxBitmapFileHeader_tail->xNew.cx;
                xBitmapInfoHeader.cy = pxBitmapFileHeader_tail->xNew.cy;
                *(PULONG)&xBitmapInfoHeader.cPlanes = *(PULONG)&pxBitmapFileHeader_tail->xNew.cPlanes;
            }
            ulBitmapDataSize = ((ULONG)xBitmapInfoHeader.cBitCount * xBitmapInfoHeader.cx + (ULONG)31) / (ULONG)32 * (ULONG)xBitmapInfoHeader.cPlanes * (ULONG)4 * xBitmapInfoHeader.cy * (ULONG)2;  /* x2 for RLE margin */
            pvBitmapData = (PVOID)_malloc((size_t)ulBitmapDataSize);
            if(DosReadComplete(hfBitmapFile,
                               pvBitmapData,
                               ulBitmapDataSize,
                               &ulActual) == NO_ERROR)
            {
                hps = GpiCreatePSMDCBFromMemory(&xBitmapInfoHeader,
                                                pvBitmapData,
                                                (PBITMAPINFO2)pxBitmapFileHeader_tail);
                if(hps != (HPS)GPI_ERROR)
                {
                    pxszBitmapCreated->cx = (LONG)xBitmapInfoHeader.cx;
                    pxszBitmapCreated->cy = (LONG)xBitmapInfoHeader.cy;
                    _free((void*)pvBitmapData);
                    _free((void*)pxBitmapFileHeader_tail);
                    return hps;
                }
            }
            _free((void*)pvBitmapData);
        }
        _free((void*)pxBitmapFileHeader_tail);
    }
    return (HPS)GPI_ERROR;
}

/*  */
HPS GpiCreatePSMDCBFromFile(PSZ pszBitmapFile,
                            PSIZEL pxszBitmapCreated)
{
    HFILE hfBitmapFile;
    ULONG ulWork;
    HPS hps;
    if(DosOpen(pszBitmapFile,
               &hfBitmapFile,
               &ulWork,
               (ULONG)0,
               FILE_NORMAL,
               OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_SEQUENTIAL | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
               (PEAOP2)NULL) == NO_ERROR)
    {
        hps = GpiCreatePSMDCBFromHFile(hfBitmapFile,
                                       pxszBitmapCreated);
        (VOID)DosClose(hfBitmapFile);
        if(hps != (HPS)GPI_ERROR)
            return hps;
    }
    return (HPS)GPI_ERROR;
}

/*  */
BOOL GpiDestroyPSMDCB(HPS hps)
{
    HBITMAP hbmp;
    HDC hdc;
    hbmp = GpiSetBitmap(hps,
                        (HBITMAP)NULLHANDLE);
    switch(hbmp)
    {
        case (HBITMAP)NULLHANDLE:
            break;
        case (HBITMAP)HBM_ERROR:
            return (BOOL)FALSE;
        default:
            if(GpiDeleteBitmap(hbmp) == (BOOL)FALSE)
                return (BOOL)FALSE;
            break;
    }
    hdc = GpiQueryDevice(hps);
    if(GpiDestroyPS(hps) == (BOOL)FALSE)
        return (BOOL)FALSE;
    switch(hdc)
    {
        case (HDC)NULLHANDLE:
            break;
        case (HDC)HDC_ERROR:
            return (BOOL)FALSE;
        default:
            if(DevCloseDC(hdc) == (HMF)DEV_ERROR)
                return (BOOL)FALSE;
            break;
    }
    return (BOOL)TRUE;
}

/*  */
MRESULT WinDefTileWindowProc(HWND hwndInstance,
                             ULONG ulMessage,
                             MPARAM mp1,
                             MPARAM mp2)
{
    HWND hwndOwner;
    switch(ulMessage)
    {
        case WM_CREATE:
            /* return (MRESULT)(BOOL)FALSE; */
        case WM_DESTROY:
        case WM_ADJUSTWINDOWPOS:
        case WM_ENABLE:
        case WM_OWNERPOSCHANGE:
        case WM_SHOW:
        case WM_WINDOWPOSCHANGED:
        case WM_QUERYFRAMEINFO:
        case WM_MOVEALONGWINDOW:
        case WM_MOVEACROSSWINDOW:
            return (MRESULT)0;  /* no operations for these frequently-sent messages, even if WinDefWindowProc()! */
        case WM_PAINT:
        {
            PSWP pswpTile;
            HPS hps;
            POINTL axptBitBlt[3];
            pswpTile = (PSWP)WinQueryWindowPtr(hwndInstance,
                                               (LONG)0);
            hps = WinBeginPaint(hwndInstance,
                                (HPS)NULLHANDLE,
                                (PRECTL)&axptBitBlt[0]);
            axptBitBlt[2].x = (LONG)((PSHORT)&pswpTile->ulReserved2)[0] + axptBitBlt[0].x;
            axptBitBlt[2].y = (LONG)((PSHORT)&pswpTile->ulReserved2)[1] + axptBitBlt[0].y;
            (VOID)GpiBitBlt(hps,
                            ((PTILEWINDOWINFO)pswpTile->ulReserved1)->hpsTileImage,
                            (LONG)3,
                            &axptBitBlt[0],
                            ROP_SRCCOPY,
                            BBO_IGNORE);
            (VOID)WinEndPaint(hps);
            return (MRESULT)0;
        }
        case WM_BEGINDRAG:
        case WM_BUTTON1CLICK:
        case WM_BUTTON1DBLCLK:
        case WM_BUTTON1DOWN:
        case WM_BUTTON1MOTIONSTART:
        case WM_BUTTON1MOTIONEND:
        case WM_BUTTON1UP:
        case WM_BUTTON2CLICK:
        case WM_BUTTON2DBLCLK:
        case WM_BUTTON2DOWN:
        case WM_BUTTON2MOTIONSTART:
        case WM_BUTTON2MOTIONEND:
        case WM_BUTTON2UP:
        case WM_BUTTON3CLICK:
        case WM_BUTTON3DBLCLK:
        case WM_BUTTON3DOWN:
        case WM_BUTTON3MOTIONSTART:
        case WM_BUTTON3MOTIONEND:
        case WM_BUTTON3UP:
        case WM_CHAR:
        case WM_CHORD:
        case WM_CONTEXTMENU:
        case WM_ENDDRAG:
        case WM_HITTEST:
        case WM_MOUSEMOVE:
            hwndOwner = WinQueryWindow(hwndInstance,
                                       QW_OWNER);
            if(hwndOwner != (HWND)NULLHANDLE)
            {
                TILEWINDOWUSEREVENT xUserEvent;
userevent:
                xUserEvent.hwndWindow = hwndInstance;
                xUserEvent.ulMessage = ulMessage;
                xUserEvent.mp1 = mp1;
                xUserEvent.mp2 = mp2;
                xUserEvent.pxTileWindowInfo = (PTILEWINDOWINFO)((PSWP)WinQueryWindowPtr(hwndInstance,
                                                                                        (LONG)0))->ulReserved1;
                return WinSendMsg(hwndOwner,
                                  WM_CONTROL,
                                  MPFROM2SHORT(WinQueryWindowUShort(hwndInstance,
                                                                    QWS_ID),
                                               TWN_USEREVENT),
                                  (MPARAM)&xUserEvent);
            }
            if(ulMessage != WM_HITTEST)
                return (MRESULT)(BOOL)FALSE;
            break;
        default:
            hwndOwner = WinQueryWindow(hwndInstance,
                                       QW_OWNER);
            if(hwndOwner != (HWND)NULLHANDLE &&
               (BOOL)WinSendMsg(hwndOwner,
                                WM_CONTROL,
                                MPFROM2SHORT(WinQueryWindowUShort(hwndInstance,
                                                                  QWS_ID),
                                             TWN_USEREVENTFILTER),
                                (MPARAM)ulMessage) != (BOOL)FALSE)
                goto userevent;
            break;
    }
    return WinDefWindowProc(hwndInstance,
                            ulMessage,
                            mp1,
                            mp2);
}

/*  */
BOOL WinRegisterTileWindow(HAB hAB)
{
    return WinRegisterClass(hAB,
                            WC_TILEWINDOW,
                            WinDefTileWindowProc,
                            CS_HITTEST | CS_SYNCPAINT | CS_CLIPSIBLINGS,
                            (ULONG)sizeof(PSWP));
}

/*  */
static int WinCreateTileWindow_qsortcallback(const void* key,
                                             const void* element)
{
    int diff = (int)((PSWP)key)->y - (int)((PSWP)element)->y;
    if(diff == 0)
        diff = (int)((PSWP)key)->x - (int)((PSWP)element)->x;
    return diff;
}

/*  */
PTILEWINDOWINFO WinCreateTileWindow(HWND hwndParent,
                                    HWND hwndOwner,
                                    ULONG ulID,
                                    HRGN hrgnShape,
                                    ULONG ulMaxElements,
                                    HPS hpsImage,
                                    PPOINTL pxptImageOffset,
                                    PHWND phwndInterior,
                                    ULONG ulInteriorCount)
{
    PRECTL pxrectInterior;
    ULONG ulIndex;
    SWP xswpWork;
    HRGN hrgnTile;
    HRGN hrgnInterior;
    RECTL xrectTileBound;
    PRECTL pxrectTiles;
    ULONG ulTileCount;
    RECTL xrectScanLine;
    RGNRECT xrgnrectControl;
    PRECTL pxrectStrip;
    LONG lY;
    ULONG ulIndex2;
    PTILEWINDOWINFO pxInfo;
#define pxswpTiles ((PSWP)&pxInfo[1])
    pxrectInterior = (PRECTL)_malloc(sizeof(RECTL) * (size_t)ulInteriorCount);
    for(ulIndex = (ULONG)0;
        ulIndex < ulInteriorCount;
        ulIndex++)
    {
        if(WinQueryWindowPos(phwndInterior[ulIndex],
                             &xswpWork) == (BOOL)FALSE)
        {
            _free((void*)pxrectInterior);
            return (PTILEWINDOWINFO)NULL;
        }
        pxrectInterior[ulIndex].xRight = (pxrectInterior[ulIndex].xLeft = xswpWork.x) + xswpWork.cx;
        pxrectInterior[ulIndex].yTop = (pxrectInterior[ulIndex].yBottom = xswpWork.y) + xswpWork.cy;
    }
    hrgnTile = GpiCreateRegion(hpsImage,
                               (LONG)0,
                               (PRECTL)NULL);
    if(hrgnTile == (HRGN)RGN_ERROR)
    {
        _free((void*)pxrectInterior);
        return (PTILEWINDOWINFO)NULL;
    }
    hrgnInterior = GpiCreateRegion(hpsImage,
                                   (LONG)ulInteriorCount,
                                   pxrectInterior);
    if(GpiCombineRegion(hpsImage,
                        hrgnTile,
                        hrgnShape,
                        hrgnInterior,
                        CRGN_DIFF) == (LONG)RGN_ERROR)
    {
        (VOID)GpiDestroyRegion(hpsImage,
                               hrgnInterior);
        (VOID)GpiDestroyRegion(hpsImage,
                               hrgnTile);
        _free((void*)pxrectInterior);
        return (PTILEWINDOWINFO)NULL;
    }
    (VOID)GpiDestroyRegion(hpsImage,
                           hrgnInterior);
    _free((void*)pxrectInterior);
    (VOID)GpiQueryRegionBox(hpsImage,
                            hrgnTile,
                            &xrectTileBound);
    if(ulMaxElements == (ULONG)0)
        ulMaxElements = MAXTILES;
    pxrectTiles = (PRECTL)_malloc(sizeof(RECTL) * ulMaxElements);
    ulTileCount = (ULONG)0;
    xrectScanLine.xLeft = xrectTileBound.xLeft;
    xrectScanLine.xRight = xrectTileBound.xRight;
    xrgnrectControl.ircStart = (ULONG)1;
    xrgnrectControl.crc = MAXSTRIPS;
    xrgnrectControl.ulDirection = RECTDIR_LFRT_BOTTOP;
    pxrectStrip = (PRECTL)_malloc(sizeof(RECTL) * MAXSTRIPS);
    for(lY = xrectTileBound.yBottom;
        lY < xrectTileBound.yTop;
        lY++)
    {
        xrectScanLine.yBottom = lY;
        xrectScanLine.yTop = lY + (LONG)1;
        (VOID)GpiQueryRegionRects(hpsImage,
                                  hrgnTile,
                                  &xrectScanLine,
                                  &xrgnrectControl,
                                  &pxrectStrip[0]);
        for(ulIndex = (ULONG)0;
            ulIndex < xrgnrectControl.crcReturned;
            ulIndex++)
        {
            for(ulIndex2 = ulTileCount;
                ulIndex2 != (ULONG)0;
                ulIndex2--)
                if(pxrectTiles[ulIndex2 - (ULONG)1].xLeft == pxrectStrip[ulIndex].xLeft &&
                   pxrectTiles[ulIndex2 - (ULONG)1].xRight == pxrectStrip[ulIndex].xRight &&
                   pxrectTiles[ulIndex2 - (ULONG)1].yTop == pxrectStrip[ulIndex].yBottom)
                {
                    pxrectTiles[ulIndex2 - (ULONG)1].yTop = pxrectStrip[ulIndex].yTop;
                    break;
                }
            if(ulIndex2 == (ULONG)0 &&
               ulTileCount < ulMaxElements)
                (void)memcpy((void*)&pxrectTiles[ulTileCount++],
                             (const void*)&pxrectStrip[ulIndex],
                             sizeof(RECTL));
        }
    }
    _free((void*)pxrectStrip);
    (VOID)GpiDestroyRegion(hpsImage,
                           hrgnTile);
    pxInfo = (PTILEWINDOWINFO)_malloc(sizeof(TILEWINDOWINFO) + sizeof(SWP) * (size_t)(ulTileCount + ulInteriorCount) * (size_t)2);
    pxInfo->ulTileCount = ulTileCount + ulInteriorCount;
    pxInfo->hpsTileImage = hpsImage;
    pxInfo->xptImageOffset = *pxptImageOffset;
    /* pxInfo->xptPosition.x = (LONG)0; */
    /* pxInfo->xptPosition.y = (LONG)0; */
    for(ulIndex = (ULONG)0;
        ulIndex < ulTileCount;
        ulIndex++)
    {
        pxswpTiles[ulIndex].cy = pxrectTiles[ulIndex].yTop - (pxswpTiles[ulIndex].y = pxrectTiles[ulIndex].yBottom);
        pxswpTiles[ulIndex].cx = pxrectTiles[ulIndex].xRight - (pxswpTiles[ulIndex].x = pxrectTiles[ulIndex].xLeft);
        pxswpTiles[ulIndex].hwndInsertBehind = HWND_TOP;
        pxswpTiles[ulIndex].ulReserved1 = (ULONG)pxInfo;
        pxswpTiles[ulIndex].ulReserved2 = ((ULONG)(pxInfo->xptImageOffset.x + pxrectTiles[ulIndex].xLeft) & (ULONG)65535) +
                                          ((ULONG)(pxInfo->xptImageOffset.y + pxrectTiles[ulIndex].yBottom) << 16);
    }
    _free((void*)pxrectTiles);
    for(ulIndex = (ULONG)0;
        ulIndex < ulInteriorCount;
        ulIndex++)
    {
        (VOID)WinQueryWindowPos(pxswpTiles[ulIndex + ulTileCount].hwnd = phwndInterior[ulIndex],
                                &pxswpTiles[ulIndex + ulTileCount]);
        pxswpTiles[ulIndex + ulTileCount].ulReserved1 = (ULONG)0;
    }
    for(ulIndex = (ULONG)0;
        ulIndex < pxInfo->ulTileCount;
        ulIndex++)
        if(pxswpTiles[ulIndex].ulReserved1 == (ULONG)0)
        {
            (VOID)WinSetParent(pxswpTiles[ulIndex].hwnd,
                               hwndParent,
                               (BOOL)FALSE);
            (VOID)WinSetOwner(pxswpTiles[ulIndex].hwnd,
                              hwndOwner);
        }
        else
            if((pxswpTiles[ulIndex].hwnd = WinCreateWindow(hwndParent,
                                                           WC_TILEWINDOW,
                                                           (PSZ)NULL,
                                                           (ULONG)0,
                                                           pxswpTiles[ulIndex].x,
                                                           pxswpTiles[ulIndex].y,
                                                           pxswpTiles[ulIndex].cx,
                                                           pxswpTiles[ulIndex].cy,
                                                           hwndOwner,
                                                           HWND_TOP,
                                                           ulID,
                                                           (PVOID)NULL,
                                                           (PVOID)NULL)) == (HWND)NULLHANDLE)
            {
                for(ulIndex2 = (ULONG)0;
                    ulIndex2 < ulIndex;
                    ulIndex2++)
                    (VOID)WinDestroyWindow(pxswpTiles[ulIndex2].hwnd);
                _free((void*)pxInfo);
                _free((void*)pxrectTiles);
                return (PTILEWINDOWINFO)NULL;
            }
    qsort((void*)pxswpTiles,
          (size_t)pxInfo->ulTileCount,
          sizeof(SWP),
          WinCreateTileWindow_qsortcallback);
    for(ulIndex = (ULONG)0;
        ulIndex < pxInfo->ulTileCount;
        ulIndex++)
        if(pxswpTiles[ulIndex].ulReserved1 != (ULONG)0)
            (VOID)WinSetWindowPtr(pxswpTiles[ulIndex].hwnd,
                                  (LONG)0,
                                  (PVOID)&pxswpTiles[ulIndex]);
    return pxInfo;
#undef pxswpTiles
}

/*  */
BOOL WinDestroyTileWindow(PTILEWINDOWINFO pxInfo,
                          BOOL bDestroyInterior)
{
#define pxswpTiles ((PSWP)&pxInfo[1])
    ULONG ulIndex;
    (VOID)WinLockWindowUpdate(HWND_DESKTOP,
                              HWND_DESKTOP);
    for(ulIndex = (ULONG)0;
        ulIndex < pxInfo->ulTileCount;
        ulIndex++)
        pxswpTiles[ulIndex].fl = SWP_HIDE;
    _SetMultWindowPos(pxswpTiles,
                      pxInfo->ulTileCount);
    for(ulIndex = (ULONG)0;
        ulIndex < pxInfo->ulTileCount;
        ulIndex++)
        if(pxswpTiles[ulIndex].ulReserved1 != (ULONG)0 ||
           bDestroyInterior != (BOOL)FALSE)
            (VOID)WinDestroyWindow(pxswpTiles[ulIndex].hwnd);
    _free((void*)pxInfo);
    (VOID)WinLockWindowUpdate(HWND_DESKTOP,
                              (HWND)NULLHANDLE);
    return (BOOL)TRUE;
#undef pxswpTiles
}

/*  */
BOOL WinSetTileWindowPos(PTILEWINDOWINFO pxInfo,
                         HWND hwndInsertBehind,
                         LONG lX,
                         LONG lY,
                         ULONG ulFlags)
{
#define pxswpTiles ((PSWP)&pxInfo[1])
    LONG xDiff, yDiff;
    BOOL bReverse = (BOOL)FALSE;
    ULONG ulIndex;
    ulFlags = (ulFlags & ~SWP_SIZE) | SWP_NOADJUST;
    if((ulFlags & SWP_MOVE) != (ULONG)0)
    {
        xDiff = lX - pxInfo->xptPosition.x;
        yDiff = lY - pxInfo->xptPosition.y;
        if(yDiff > (LONG)0 ||
           yDiff == (LONG)0 && xDiff > (LONG)0)
            bReverse = (BOOL)TRUE;
        for(ulIndex = (ULONG)0;
            ulIndex < pxInfo->ulTileCount;
            ulIndex++)
        {
            pxswpTiles[ulIndex].hwndInsertBehind = hwndInsertBehind;
            pxswpTiles[ulIndex].x += xDiff;
            pxswpTiles[ulIndex].y += yDiff;
            pxswpTiles[ulIndex].fl = ulFlags;
        }
        pxInfo->xptPosition.x = lX;
        pxInfo->xptPosition.y = lY;
    }
    else
        for(ulIndex = (ULONG)0;
            ulIndex < pxInfo->ulTileCount;
            ulIndex++)
        {
            pxswpTiles[ulIndex].hwndInsertBehind = hwndInsertBehind;
            pxswpTiles[ulIndex].fl = ulFlags;
        }
    if(bReverse != (BOOL)FALSE)
    {
        for(ulIndex = (ULONG)0;
            ulIndex < pxInfo->ulTileCount;
            ulIndex++)
            (void)memcpy((void*)&pxswpTiles[pxInfo->ulTileCount * (ULONG)2 - (ULONG)1 - ulIndex],
                         (const void*)&pxswpTiles[ulIndex],
                         sizeof(SWP));
        _SetMultWindowPos(&pxswpTiles[pxInfo->ulTileCount],
                          pxInfo->ulTileCount);
    }
    else
        _SetMultWindowPos(pxswpTiles,
                          pxInfo->ulTileCount);
    return (BOOL)TRUE;
#undef pxswpTiles
}

/*  */
BOOL WinQueryTileWindowPos(PTILEWINDOWINFO pxInfo,
                           PSWP pxswpPosition)
{
#define pxswpTiles ((PSWP)&pxInfo[1])
    if(WinQueryWindowPos(pxswpTiles[0].hwnd,
                         pxswpPosition) != (BOOL)FALSE)
    {
        pxswpPosition->x = pxInfo->xptPosition.x;
        pxswpPosition->y = pxInfo->xptPosition.y;
        pxswpPosition->fl &= ~SWP_SIZE;
        pxswpPosition->cy = pxswpPosition->cx = (LONG)0;
        return (BOOL)TRUE;
    }
    return (BOOL)FALSE;
#undef pxswpTiles
}

/*  */
BOOL WinInvalidateTileWindowRect(PTILEWINDOWINFO pxInfo,
                                 PRECTL pxrectInvalidated)
{
#define pxswpTiles ((PSWP)&pxInfo[1])
    ULONG ulIndex;
    if(pxrectInvalidated == (PRECTL)NULL)
        for(ulIndex = (ULONG)0;
            ulIndex < pxInfo->ulTileCount;
            ulIndex++)
            (VOID)WinInvalidateRect(pxswpTiles[ulIndex].hwnd,
                                    (PRECTL)NULL,
                                    (BOOL)TRUE);
    else
    {
        HAB hAB = WinQueryAnchorBlock(HWND_DESKTOP);
        for(ulIndex = (ULONG)0;
            ulIndex < pxInfo->ulTileCount;
            ulIndex++)
        {
            RECTL xrectTile, xrectIntersect;
            xrectTile.xRight = (xrectTile.xLeft = pxswpTiles[ulIndex].x - pxInfo->xptPosition.x) + pxswpTiles[ulIndex].cx;
            xrectTile.yTop = (xrectTile.yBottom = pxswpTiles[ulIndex].y - pxInfo->xptPosition.y) + pxswpTiles[ulIndex].cy;
            if(WinIntersectRect(hAB,
                                &xrectIntersect,
                                &xrectTile,
                                pxrectInvalidated) != (BOOL)FALSE)
            {
                xrectIntersect.xLeft -= xrectTile.xLeft;
                xrectIntersect.yBottom -= xrectTile.yBottom;
                xrectIntersect.xRight -= xrectTile.xLeft;
                xrectIntersect.yTop -= xrectTile.yBottom;
                (VOID)WinInvalidateRect(pxswpTiles[ulIndex].hwnd,
                                        &xrectIntersect,
                                        (BOOL)TRUE);
            }
        }
    }
    return (BOOL)TRUE;
#undef pxswpTiles
}

/*  */
BOOL WinEnableTileWindow(PTILEWINDOWINFO pxInfo,
                         BOOL bEnabled)
{
#define pxswpTiles ((PSWP)&pxInfo[1])
    ULONG ulIndex;
    for(ulIndex = (ULONG)0;
        ulIndex < pxInfo->ulTileCount;
        ulIndex++)
        (VOID)WinEnableWindow(pxswpTiles[ulIndex].hwnd,
                              bEnabled);
    return (BOOL)TRUE;
#undef pxswpTiles
}

/*  */
BOOL WinAdjustTileWindowPosFromOwner(PTILEWINDOWINFO pxInfo,
                                     HWND hwndOwner,
                                     PSWP pxswpOwnerNew)
{
    SWP xswpOwnerOld;
    SWP xswpTileWindow;
    (VOID)WinQueryWindowPos(hwndOwner,
                            &xswpOwnerOld);
    (VOID)WinQueryTileWindowPos(pxInfo,
                                &xswpTileWindow);
    xswpTileWindow.fl = (ULONG)0;
    if((pxswpOwnerNew->fl & (SWP_MOVE | SWP_SIZE)) != (ULONG)0)
    {
        xswpTileWindow.x += pxswpOwnerNew->x - xswpOwnerOld.x;
        xswpTileWindow.y += pxswpOwnerNew->y - xswpOwnerOld.y;
        if((pxswpOwnerNew->fl & SWP_SIZE) != (ULONG)0)
            xswpTileWindow.y += pxswpOwnerNew->cy - xswpOwnerOld.cy;
        xswpTileWindow.fl |= SWP_MOVE;
    }
    if((pxswpOwnerNew->fl & (SWP_SHOW | SWP_MAXIMIZE | SWP_RESTORE)) != (ULONG)0)
        xswpTileWindow.fl |= SWP_SHOW;
    if((pxswpOwnerNew->fl & (SWP_HIDE | SWP_MINIMIZE)) != (ULONG)0)
        xswpTileWindow.fl |= SWP_HIDE;
    (VOID)WinSetTileWindowPos(pxInfo,
                              (HWND)NULLHANDLE,
                              xswpTileWindow.x,
                              xswpTileWindow.y,
                              xswpTileWindow.fl);
    pxswpOwnerNew->fl &= ~SWP_SHOW;
    return (BOOL)TRUE;
}

