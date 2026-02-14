/**
 * MeshXTCompress — Compression for Meshtastic satellite relay
 * © Mikoshi Ltd. — Apache 2.0
 */

#ifndef MESHXT_COMPRESS_H
#define MESHXT_COMPRESS_H

#include <stdint.h>

#define MESHXT_MAGIC_0  0x4D  // 'M'
#define MESHXT_MAGIC_1  0x58  // 'X'
#define MESHXT_HEADER_SIZE 2

class MeshXTCompress {
public:
    MeshXTCompress();

    /**
     * Compress a message payload.
     * @param input   Raw message bytes
     * @param inLen   Input length
     * @param output  Compressed output buffer (must be >= inLen + MESHXT_HEADER_SIZE)
     * @param outLen  Output length (set on success)
     * @return 0 on success, negative on error
     */
    int compress(const uint8_t *input, uint16_t inLen, uint8_t *output, uint16_t &outLen);

    /**
     * Decompress a MeshXT-compressed payload.
     * @param input   Compressed bytes (with MeshXT header)
     * @param inLen   Input length
     * @param output  Decompressed output buffer
     * @param outLen  Output length (set on success)
     * @return 0 on success, negative on error
     */
    int decompress(const uint8_t *input, uint16_t inLen, uint8_t *output, uint16_t &outLen);

private:
    // Simple dictionary for common Meshtastic words
    struct DictEntry {
        const char *word;
        uint8_t     code;
    };

    static const DictEntry dictionary[];
    static const uint8_t   dictSize;

    int  dictCompress(const uint8_t *in, uint16_t inLen, uint8_t *out, uint16_t &outLen);
    int  dictDecompress(const uint8_t *in, uint16_t inLen, uint8_t *out, uint16_t &outLen);
    int  rleCompress(const uint8_t *in, uint16_t inLen, uint8_t *out, uint16_t &outLen);
    int  rleDecompress(const uint8_t *in, uint16_t inLen, uint8_t *out, uint16_t &outLen);
};

#endif // MESHXT_COMPRESS_H
