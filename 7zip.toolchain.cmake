project(l7z_source)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

if(BUILD_PLATFORM MATCHES "ios")
    SET(lib_name nice7z_${ARCHS})
else()
    SET(lib_name nice7z)
endif()

add_library(
        ${lib_name}
        SHARED
        extract.c
        extract.h

        7zip/C/LzmaDec.h
        7zip/C/LzmaDec.c
        7zip/C/Alloc.c
        7zip/C/Alloc.h

        7zip/C/CpuArch.h
        7zip/C/CpuArch.c
        7zip/C/7z.h
        7zip/C/7zAlloc.c
        7zip/C/7zAlloc.h
        7zip/C/7zArcIn.c
        7zip/C/7zBuf.c
        7zip/C/7zBuf.h
        7zip/C/7zBuf2.c
        7zip/C/7zCrc.c
        7zip/C/7zCrc.h
        7zip/C/7zCrcOpt.c
        7zip/C/7zDec.c
        7zip/C/7zFile.c
        7zip/C/7zFile.h
        7zip/C/7zStream.c
        7zip/C/7zTypes.h
        7zip/C/7zVersion.h
        7zip/C/7zVersion.rc
        7zip/C/Bcj2.h
        7zip/C/Bcj2.c
        7zip/C/Bra.h
        7zip/C/Bra.c
        7zip/C/Bra86.c
        7zip/C/BraIA64.c
        7zip/C/Delta.h
        7zip/C/Delta.c
        7zip/C/Lzma2Dec.c
        7zip/C/Lzma2Dec.h
        7zip/C/Lzma86Dec.c
)
