# makefile - "TileWindow" free-shape window library, public release 1.1.3 (build 13)
#   Copyright (c) 1999-2002 Takayuki 'January June' Suwa
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

..\TILEWND.lib : ..\TILEWND.dll
    implib ..\TILEWND.lib ..\TILEWND.dll

..\TILEWND.dll : TILEWND.obj TILEWND.def
    ilink /NOFREE /OPTFUNC /PACKCODE /PACKDATA /NOLOGO TILEWND.obj, ..\TILEWND.dll, nul, , TILEWND.def
    eautil ..\TILEWND.dll nul /s
    lxlite ..\TILEWND.dll /b- /c+ /i- /l- /mr3 /mln /mf1 /s+ /t: /v- /yur /ydd /yxs /ynl /zs:1024

TILEWND.obj : TILEWND.c ..\TILEWND.h TILEWND.def
    icc /I.. /G5 /Gs+ /Gi+ /Gf+ /Gu+ /O+ /Oc+ /Oi- /Ol+ /Rn /Q+ /C+ TILEWND.c TILEWND.def

