/* extract.h -- extract 7z interface
2023-02-16 : Julien Dufresne - Recursyve Solutions */

#ifndef L7Z_EXTRACT_H
#define L7Z_EXTRACT_H

#define kInputBufSize ((size_t)1 << 18)

int extract_7z_archive(const char* source, const char* dest, void progress(double));

#endif //L7Z_EXTRACT_H
