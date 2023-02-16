/* extract.h -- extract 7z source
2023-02-16 : Julien Dufresne - Recursyve Solutions
From https://gitlab.com/7zip.dev/7zip/-/blob/master/C/Util/7z/7zMain.c
 */

#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "extract.h"

#include "7zTypes.h"
#include "7zFile.h"
#include "7z.h"
#include "Alloc.h"
#include "7zCrc.h"
#include "7zAlloc.h"
#include "7zBuf.h"

static int Buf_EnsureSize(CBuf *dest, size_t size)
{
	if (dest->size >= size)
		return 1;
	Buf_Free(dest, &g_Alloc);
	return Buf_Create(dest, size, &g_Alloc);
}

#ifndef _WIN32
#define _USE_UTF8
#endif

/* #define _USE_UTF8 */

#ifdef _USE_UTF8

#define _UTF8_START(n) (0x100 - (1 << (7 - (n))))

#define _UTF8_RANGE(n) (((UInt32)1) << ((n) * 5 + 6))

#define _UTF8_HEAD(n, val) ((Byte)(_UTF8_START(n) + (val >> (6 * (n)))))
#define _UTF8_CHAR(n, val) ((Byte)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

static size_t Utf16_To_Utf8_Calc(const UInt16 *src, const UInt16 *srcLim)
{
	size_t size = 0;
	for (;;)
	{
		UInt32 val;
		if (src == srcLim)
			return size;

		size++;
		val = *src++;

		if (val < 0x80)
			continue;

		if (val < _UTF8_RANGE(1))
		{
			size++;
			continue;
		}

		if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
		{
			UInt32 c2 = *src;
			if (c2 >= 0xDC00 && c2 < 0xE000)
			{
				src++;
				size += 3;
				continue;
			}
		}

		size += 2;
	}
}

static Byte *Utf16_To_Utf8(Byte *dest, const UInt16 *src, const UInt16 *srcLim)
{
	for (;;)
	{
		UInt32 val;
		if (src == srcLim)
			return dest;

		val = *src++;

		if (val < 0x80)
		{
			*dest++ = (char)val;
			continue;
		}

		if (val < _UTF8_RANGE(1))
		{
			dest[0] = _UTF8_HEAD(1, val);
			dest[1] = _UTF8_CHAR(0, val);
			dest += 2;
			continue;
		}

		if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
		{
			UInt32 c2 = *src;
			if (c2 >= 0xDC00 && c2 < 0xE000)
			{
				src++;
				val = (((val - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
				dest[0] = _UTF8_HEAD(3, val);
				dest[1] = _UTF8_CHAR(2, val);
				dest[2] = _UTF8_CHAR(1, val);
				dest[3] = _UTF8_CHAR(0, val);
				dest += 4;
				continue;
			}
		}

		dest[0] = _UTF8_HEAD(2, val);
		dest[1] = _UTF8_CHAR(1, val);
		dest[2] = _UTF8_CHAR(0, val);
		dest += 3;
	}
}

static SRes Utf16_To_Utf8Buf(CBuf *dest, const UInt16 *src, size_t srcLen)
{
	size_t destLen = Utf16_To_Utf8_Calc(src, src + srcLen);
	destLen += 1;
	if (!Buf_EnsureSize(dest, destLen))
		return SZ_ERROR_MEM;
	*Utf16_To_Utf8(dest->data, src, src + srcLen) = 0;
	return SZ_OK;
}

#endif

static SRes Utf16_To_Char(CBuf *buf, const UInt16 *s
#ifndef _USE_UTF8
	, UINT codePage
#endif
)
{
	unsigned len = 0;
	for (len = 0; s[len] != 0; len++);

#ifndef _USE_UTF8
	{
    unsigned size = len * 3 + 100;
    if (!Buf_EnsureSize(buf, size))
      return SZ_ERROR_MEM;
    {
      buf->data[0] = 0;
      if (len != 0)
      {
        char defaultChar = '_';
        BOOL defUsed;
        unsigned numChars = 0;
        numChars = WideCharToMultiByte(codePage, 0, (LPCWSTR)s, len, (char *)buf->data, size, &defaultChar, &defUsed);
        if (numChars == 0 || numChars >= size)
          return SZ_ERROR_FAIL;
        buf->data[numChars] = 0;
      }
      return SZ_OK;
    }
  }
#else
	return Utf16_To_Utf8Buf(buf, s, len);
#endif
}

#ifdef _WIN32
#ifndef USE_WINDOWS_FILE
    static UINT g_FileCodePage = CP_ACP;
  #endif
  #define MY_FILE_CODE_PAGE_PARAM ,g_FileCodePage
#else
#define MY_FILE_CODE_PAGE_PARAM
#endif

int validate_dest(const char* dest) {
	struct stat stats;

	if (stat(dest, &stats) == 0 && S_ISDIR(stats.st_mode)) {
		return 1;
	} else {
		return 0;
	}
}

static WRes create_dir(const char* path, const UInt16* name, size_t pathLen) {
	CBuf buf;
	Buf_Init(&buf);
	RINOK(Utf16_To_Char(&buf, name MY_FILE_CODE_PAGE_PARAM));

	char dir[256] = "";
	strcat(dir, path);
	if (dir[pathLen - 1] != '/') {
		strcat(dir, "/");
	}

	char* subpath = strtok((char*) buf.data, "/");
	while (subpath != NULL) {
		strcat(dir, subpath);
		strcat(dir, "/");

		if (validate_dest(dir) == 0)
		{
			if (mkdir(dir, 0777) != 0)
			{
				return errno;
			}
		}

		subpath = strtok(NULL, "/");
	}

	Buf_Free(&buf, &g_Alloc);

	return 0;
}

static WRes open_file(const char* path, const UInt16* name, size_t pathLen, CSzFile* outFile) {
	CBuf buf;
	Buf_Init(&buf);
	RINOK(Utf16_To_Char(&buf, name MY_FILE_CODE_PAGE_PARAM));

	char filename[256] = "";
	strcat(filename, path);
	if (filename[pathLen - 1] != '/') {
		strcat(filename, "/");
	}

	strcat(filename, (char*) buf.data);

	Buf_Free(&buf, &g_Alloc);

	if (OutFile_Open(outFile, filename)) {
		return SZ_ERROR_FAIL;
	}

	return SZ_OK;
}

int extract_7z_archive(const char* source, const char* dest, void progress(double)) {
	if (validate_dest(dest) == 0) {
		return -1;
	}

	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	CFileInStream archiveStream;
	CLookToRead2 lookStream;
	CSzArEx db;
	SRes res;
	UInt16 *temp = NULL;
	size_t tempSize = 0;

	size_t destFullLen = strlen(dest);

	allocImp = g_Alloc;
	allocTempImp = g_Alloc;

	res = SZ_OK;

	res = InFile_Open(&archiveStream.file, source);
	if (res != SZ_OK) {
		return res;
	}

	FileInStream_CreateVTable(&archiveStream);
	LookToRead2_CreateVTable(&lookStream, False);
	lookStream.buf = NULL;

	lookStream.buf = (Byte*)ISzAlloc_Alloc(&allocImp, kInputBufSize);
	if (!lookStream.buf) {
		res = SZ_ERROR_MEM;
	} else {
		lookStream.bufSize = kInputBufSize;
		lookStream.realStream = &archiveStream.vt;
		LookToRead2_Init(&lookStream);
	}

	CrcGenerateTable();

	SzArEx_Init(&db);

	if (res == SZ_OK)
	{
		res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
	}

	if (res == SZ_OK)
	{
		UInt32 i;

		/*
		if you need cache, use these 3 variables.
		if you use external function, you can make these variable as static.
		*/
		UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
		Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
		size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */

		for (i = 0; i < db.NumFiles; i++)
		{
			size_t offset = 0;
			size_t outSizeProcessed = 0;
			// const CSzFileItem *f = db.Files + i;
			size_t len;
			unsigned isDir = SzArEx_IsDir(&db, i);
			/*if (listCommand == 0 && isDir && !fullPaths)
				continue;*/
			len = SzArEx_GetFileNameUtf16(&db, i, NULL);
			// len = SzArEx_GetFullNameLen(&db, i);

			if (len > tempSize)
			{
				SzFree(NULL, temp);
				tempSize = len;
				temp = (UInt16 *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
				if (!temp)
				{
					res = SZ_ERROR_MEM;
					break;
				}
			}

			SzArEx_GetFileNameUtf16(&db, i, temp);
			if (!isDir) {
				res = SzArEx_Extract(
					&db,
					&lookStream.vt,
					i,
					&blockIndex,
					&outBuffer,
					&outBufferSize,
					&offset,
					&outSizeProcessed,
					&allocImp,
					&allocTempImp
				);
				if (res != SZ_OK)
					break;
			}

			CSzFile outFile;
			size_t processedSize;
			size_t j;
			UInt16 *name = (UInt16 *)temp;
			const UInt16 *d = (const UInt16 *)name;

			if (isDir)
			{
				create_dir(dest, d, destFullLen);
				progress(((double)i / db.NumFiles) * 100);
				continue;
			}

			res = open_file(dest, d, destFullLen, &outFile);
			if (res != SZ_OK)
			{
				break;
			}

			processedSize = outSizeProcessed;

			if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed)
			{
				res = SZ_ERROR_FAIL;
				break;
			}


			if (File_Close(&outFile))
			{
				res = SZ_ERROR_FAIL;
				break;
			}

			progress(((double)i / db.NumFiles) * 100);
		}
		ISzAlloc_Free(&allocImp, outBuffer);
	}

	SzFree(NULL, temp);
	SzArEx_Free(&db, &allocImp);
	ISzAlloc_Free(&allocImp, lookStream.buf);

	File_Close(&archiveStream.file);

	return res;
}
