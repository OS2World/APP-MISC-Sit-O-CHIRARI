/* TILEWND.h - "TileWindow" free-shape window library, public release 1.1.3 (build 13)
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

#if !defined(__TILEWINDOW)
#define __TILEWINDOW

#if defined(__cplusplus)
extern "C" {
#endif

/* version information */
#pragma pack(1)
extern const struct __TileWindowVersionInfo_t
{
    UCHAR aucVersion[3];  /* {1, 1, 3} in this release */
    ULONG ulBuild;        /* build 13 */
} __TileWindowVersionInfo;
#pragma pack()

/* +--------------------------+
   | some GPI-related helpers |
   +--------------------------+ */

/* creates GPI region from mask bitmap
   return : valid region handle (associated to hpsRegion) if successful
            RGN_ERROR if failed */
extern HRGN APIENTRY GpiCreateRegionFromMaskBitmap(HPS hpsRegion,       /* PS handle used for region creation */
                                                   HPS hpsMask,         /* PS handle that contains mask bitmap */
                                                   PRECTL pxrectMask,   /* pointer to mask bitmap area rectangle, can be NULL (implies hpsMask page rectangle) */
                                                   LONG lMaskColorRGB,  /* mask color in RGB */
                                                   ULONG ulMaxRects     /* # of maximum rectangles, can be 0 (implies 8192) */);

/* special mask color for GpiCreateRegionFromMaskBitmap() */
#define MASKCOLOR_LEFTBOTTOM  -1L  /* color of left-bottom pel in pxrectMask */
#define MASKCOLOR_RIGHTBOTTOM -2L  /* color of right-bottom pel in pxrectMask */
#define MASKCOLOR_NOMASKING   -3L  /* no masking (rectanglar region : whole area of pxrectMask) */

/* creates blank "micro PS with memory DC & bitmap"
   return : appropriate PS handle if successful
            GPI_ERROR if failed */
extern HPS APIENTRY GpiCreatePSMDCB(PBITMAPINFOHEADER2 pxBitmapInfoHeader  /* pointer to bitmap information used for bitmap creation */);

/* creates "micro PS with memory DC & bitmap" from memory bitmap image
   return : appropriate PS handle if successful
            GPI_ERROR if failed */
extern HPS APIENTRY GpiCreatePSMDCBFromMemory(PBITMAPINFOHEADER2 pxBitmapInfoHeader,  /* pointer to bitmap information used for bitmap creation */
                                              PVOID pvMemory,                         /* pointer to memory bitmap image */
                                              PBITMAPINFO2 pxMemoryBitmapInfo         /* pointer to bitmap information of memory bitmap image */);

/* creates "micro PS with memory DC & bitmap" from module resource
   return : appropriate PS handle if successful
            GPI_ERROR if failed */
extern HPS APIENTRY GpiCreatePSMDCBFromResource(HMODULE hmodResource,     /* module handle that contains bitmap resource, can be NULLHANDLE (implies current executable module) */
                                                ULONG ulBitmapID,         /* bitmap resource identifier */
                                                PSIZEL pxszBitmapCreated  /* pointer to variable that size of bitmap created will be set to */);

/* creates "micro PS with memory DC & bitmap" from file
   return : appropriate PS handle if successful
            GPI_ERROR if failed */
extern HPS APIENTRY GpiCreatePSMDCBFromFile(PSZ pszFileName,          /* pointer to ASCIZ bitmap file name */
                                            PSIZEL pxszBitmapCreated  /* pointer to variable that size of bitmap created will be set to */);

/* destroys "micro PS with memory DC & bitmap"
   return : TRUE if successful */
extern BOOL APIENTRY GpiDestroyPSMDCB(HPS hps  /* handle of "micro PS with memory DC & bitmap" */);

/* +----------------------------------------+
   | "TileWindow" free-shape window library |
   +----------------------------------------+ */

/* "TileWindow" structure (for internal use) */
typedef struct
{
    ULONG ulTileCount;
    HPS hpsTileImage;
    POINTL xptImageOffset;
    POINTL xptPosition;
} TILEWINDOWINFO, * PTILEWINDOWINFO;

/* "TileWindow" WM_CONTROL notification code */
#define TWN_USEREVENT       1  /* user-event notification */
#define TWN_USEREVENTFILTER 2  /* user-event filtering notification */

/* "TileWindow" WM_CONTROL/TWN_USEREVENT notified information (pointed by mp2) */
typedef struct
{
    HWND hwndWindow;  /* one of "TileWindow" elements that notified user-event */
    ULONG ulMessage;  /* user-event window message */
    MPARAM mp1;       /* user-event parameter 1 */
    MPARAM mp2;       /* user-event parameter 2 */
    PTILEWINDOWINFO pxTileWindowInfo;
                      /* pointer to parent "TileWindow" structure */
} TILEWINDOWUSEREVENT, * PTILEWINDOWUSEREVENT;

/* "TileWindow" element window class name */
#define WC_TILEWINDOW ((PSZ)"TwnTileWindow")

/* "TileWindow" element default window procedure */
extern MRESULT EXPENTRY WinDefTileWindowProc(HWND hwndInstance,  /* window handle */
                                             ULONG ulMessage,    /* window message */
                                             MPARAM mp1,         /* parameter 1 */
                                             MPARAM mp2          /* parameter 2 */);

/* registers "TileWindow" element window class
   return : result of WinRegisterClass() */
extern BOOL APIENTRY WinRegisterTileWindow(HAB hAB  /* anchor block handle */);

/* creates "TileWindow"
   return : pointer to "TileWindow" structure if successful
            NULL if failed */
extern PTILEWINDOWINFO APIENTRY WinCreateTileWindow(HWND hwndParent,          /* parent window handle of each "TileWindow" element */
                                                    HWND hwndOwner,           /* owner window handle of each "TileWindow" element, can be NULLHANDLE */
                                                    ULONG ulID,               /* window identifier of each "TileWindow" element */
                                                    HRGN hrgnShape,           /* region handle that determines "TileWindow" shape */
                                                    ULONG ulMaxElements,      /* # of maximum "TileWindow" elements, can be 0 (implies 8192) */
                                                    HPS hpsImage,             /* PS handle that contains bitmap image you want to draw inside of "TileWindow" */
                                                    PPOINTL pxptImageOffset,  /* pointer to offset of drawing "TileWindow" with hpsImage from (0, 0) */
                                                    PHWND phwndInterior,      /* pointer to array of interior window handles that you want to include to "TileWindow" */
                                                    ULONG ulInteriorCount     /* # of interior window handles you want to include to "TileWindow", can be 0 */);

/* destroys "TileWindow"
   return : TRUE if successful */
extern BOOL APIENTRY WinDestroyTileWindow(PTILEWINDOWINFO pxInfo,  /* pointer to "TileWindow" structure */
                                          BOOL bDestroyInterior    /* !FALSE if you want to destroy included interior windows at creation of "TileWindow" */);

/* sets position/visibility/Z-order of "TileWindow"
   return : TRUE if successful */
extern BOOL APIENTRY WinSetTileWindowPos(PTILEWINDOWINFO pxInfo,  /* pointer to "TileWindow" structure */
                                         HWND hwndInsertBehind,   /* same meaning of WinSetWindowPos() */
                                         LONG lX,                 /* same meaning of WinSetWindowPos() */
                                         LONG lY,                 /* same meaning of WinSetWindowPos() */
                                         ULONG ulFlags            /* same meaning of WinSetWindowPos(), excepted for size-related flags */);

/* queries position/visibility/Z-order of "TileWindow"
   return : TRUE if successful */
extern BOOL APIENTRY WinQueryTileWindowPos(PTILEWINDOWINFO pxInfo,  /* pointer to "TileWindow" structure */
                                           PSWP pxswpPosition       /* pointer to SWP structure that result will be set to */);

/* invalidates whole/part of "TileWindow"
   return : TRUE if successful */
extern BOOL APIENTRY WinInvalidateTileWindowRect(PTILEWINDOWINFO pxInfo,   /* pointer to "TileWindow" structure */
                                                 PRECTL pxrectInvalidated  /* pointer to invalidation area rectangle, can be NULL (implies whole area) */);

/* enables/disables "TileWindow" user-event handling
   return : TRUE if successful */
extern BOOL APIENTRY WinEnableTileWindow(PTILEWINDOWINFO pxInfo,  /* pointer to "TileWindow" structure */
                                         BOOL bEnabled            /* new user-event handling state */);

/* adjusts "TileWindow" position/visibility with owner (frame window) state,
     called from WM_ADJUSTFRAMEPOS section in subclassed owner window procedure
   return : TRUE if successful */
extern BOOL APIENTRY WinAdjustTileWindowPosFromOwner(PTILEWINDOWINFO pxInfo,  /* pointer to "TileWindow" structure */
                                                     HWND hwndOwner,          /* owner window handle */
                                                     PSWP pxswpOwnerNew       /* pointer to SWP structure, new owner window position/visibility state */);

#if defined(__cplusplus)
}
#endif

#endif

