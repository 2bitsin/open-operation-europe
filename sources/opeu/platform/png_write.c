/* Minimal dependency-free PNG writer.
 *
 * Emits the signature, IHDR (8-bit truecolour), one IDAT holding a zlib
 * stream made of stored (uncompressed) deflate blocks, and IEND.  Every
 * scanline uses filter type 0.  Plain C99. */
#include "png_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crc_table[256];
static int crc_ready;

static void crc_init(void)
{
    uint32_t n, c;
    int k;
    for (n = 0; n < 256; n++) {
        c = n;
        for (k = 0; k < 8; k++)
            c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
    crc_ready = 1;
}

static uint32_t crc_update(uint32_t crc, const uint8_t *p, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        crc = crc_table[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    return crc;
}

static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static int write_chunk(FILE *f, const char *type, const uint8_t *data, size_t len)
{
    uint8_t hdr[8], tail[4];
    uint32_t crc;
    put_be32(hdr, (uint32_t)len);
    memcpy(hdr + 4, type, 4);
    crc = crc_update(0xFFFFFFFFu, hdr + 4, 4);
    if (len)
        crc = crc_update(crc, data, len);
    put_be32(tail, crc ^ 0xFFFFFFFFu);
    if (fwrite(hdr, 1, 8, f) != 8) return -1;
    if (len && fwrite(data, 1, len, f) != len) return -1;
    if (fwrite(tail, 1, 4, f) != 4) return -1;
    return 0;
}

int png_write_rgb(const char *path, const uint8_t *rgb, int w, int h)
{
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    uint8_t ihdr[13];
    size_t row, raw_len, nblocks, z_len, pos, left, i;
    uint8_t *raw, *z;
    uint32_t a = 1, b = 0;
    FILE *f;
    int y, rc = -1;

    if (!path || !rgb || w <= 0 || h <= 0)
        return -1;
    if (!crc_ready)
        crc_init();

    row = (size_t)w * 3u;
    raw_len = (row + 1u) * (size_t)h;
    raw = (uint8_t *)malloc(raw_len);
    if (!raw)
        return -1;
    for (y = 0; y < h; y++) {
        raw[(size_t)y * (row + 1u)] = 0; /* filter: none */
        memcpy(raw + (size_t)y * (row + 1u) + 1u, rgb + (size_t)y * row, row);
    }

    nblocks = (raw_len + 65534u) / 65535u;
    z_len = 2u + nblocks * 5u + raw_len + 4u;
    z = (uint8_t *)malloc(z_len);
    if (!z) {
        free(raw);
        return -1;
    }
    z[0] = 0x78; /* CMF: deflate, 32K window */
    z[1] = 0x01; /* FLG: (0x78 * 256 + 0x01) % 31 == 0 */
    pos = 2;
    left = raw_len;
    i = 0;
    while (left > 0) {
        size_t n = left > 65535u ? 65535u : left;
        z[pos++] = (uint8_t)(n == left ? 1 : 0); /* BFINAL, BTYPE = 00 */
        z[pos++] = (uint8_t)(n & 0xFFu);
        z[pos++] = (uint8_t)(n >> 8);
        z[pos++] = (uint8_t)(~n & 0xFFu);
        z[pos++] = (uint8_t)((~n >> 8) & 0xFFu);
        memcpy(z + pos, raw + i, n);
        pos += n;
        i += n;
        left -= n;
    }
    for (i = 0; i < raw_len; i++) {
        a = (a + raw[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    put_be32(z + pos, (b << 16) | a);
    pos += 4;

    put_be32(ihdr, (uint32_t)w);
    put_be32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8;  /* bit depth */
    ihdr[9] = 2;  /* colour type: truecolour */
    ihdr[10] = 0; /* compression */
    ihdr[11] = 0; /* filter */
    ihdr[12] = 0; /* interlace */

    f = fopen(path, "wb");
    if (f) {
        if (fwrite(sig, 1, 8, f) == 8 &&
            write_chunk(f, "IHDR", ihdr, 13) == 0 &&
            write_chunk(f, "IDAT", z, pos) == 0 &&
            write_chunk(f, "IEND", NULL, 0) == 0)
            rc = 0;
        if (fclose(f) != 0)
            rc = -1;
    }
    free(z);
    free(raw);
    return rc;
}
