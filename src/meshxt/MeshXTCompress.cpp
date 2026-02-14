/**
 * MeshXTCompress — Compression for Meshtastic satellite relay
 * © Mikoshi Ltd. — Apache 2.0
 */

#include "MeshXTCompress.h"
#include <string.h>

// Common Meshtastic message words — single-byte codes
const MeshXTCompress::DictEntry MeshXTCompress::dictionary[] = {
    {"SOS",       0x01},
    {"HELP",      0x02},
    {"OK",        0x03},
    {"YES",       0x04},
    {"NO",        0x05},
    {"POSITION",  0x06},
    {"BATTERY",   0x07},
    {"LOW",       0x08},
    {"CHECK",     0x09},
    {"SAFE",      0x0A},
    {"MOVING",    0x0B},
    {"STOPPED",   0x0C},
    {"CAMP",      0x0D},
    {"WATER",     0x0E},
    {"EMERGENCY", 0x0F},
    {"WEATHER",   0x10},
    {"CLEAR",     0x11},
    {"STORM",     0x12},
    {"TRAIL",     0x13},
    {"NORTH",     0x14},
    {"SOUTH",     0x15},
    {"EAST",      0x16},
    {"WEST",      0x17},
    {"COPY",      0x18},
    {"ROGER",     0x19},
    {"OVER",      0x1A},
    {"OUT",       0x1B},
};

const uint8_t MeshXTCompress::dictSize = sizeof(dictionary) / sizeof(dictionary[0]);

MeshXTCompress::MeshXTCompress() {}

int MeshXTCompress::compress(const uint8_t *input, uint16_t inLen,
                              uint8_t *output, uint16_t &outLen) {
    if (inLen == 0) return -1;

    // Add MeshXT header
    output[0] = MESHXT_MAGIC_0;
    output[1] = MESHXT_MAGIC_1;

    // Try dictionary compression first
    uint16_t compLen;
    int result = dictCompress(input, inLen, output + MESHXT_HEADER_SIZE, compLen);

    if (result == 0 && compLen < inLen) {
        outLen = MESHXT_HEADER_SIZE + compLen;
        return 0;
    }

    // Fallback: RLE compression
    result = rleCompress(input, inLen, output + MESHXT_HEADER_SIZE, compLen);
    if (result == 0 && compLen < inLen) {
        outLen = MESHXT_HEADER_SIZE + compLen;
        return 0;
    }

    // No compression benefit — store raw with header
    memcpy(output + MESHXT_HEADER_SIZE, input, inLen);
    outLen = MESHXT_HEADER_SIZE + inLen;
    return 0;
}

int MeshXTCompress::decompress(const uint8_t *input, uint16_t inLen,
                                uint8_t *output, uint16_t &outLen) {
    if (inLen < MESHXT_HEADER_SIZE) return -1;
    if (input[0] != MESHXT_MAGIC_0 || input[1] != MESHXT_MAGIC_1) return -2;

    const uint8_t *data = input + MESHXT_HEADER_SIZE;
    uint16_t dataLen = inLen - MESHXT_HEADER_SIZE;

    // Try dictionary decompression
    if (dictDecompress(data, dataLen, output, outLen) == 0) {
        return 0;
    }

    // Try RLE decompression
    if (rleDecompress(data, dataLen, output, outLen) == 0) {
        return 0;
    }

    // Raw passthrough
    memcpy(output, data, dataLen);
    outLen = dataLen;
    return 0;
}

int MeshXTCompress::dictCompress(const uint8_t *in, uint16_t inLen,
                                  uint8_t *out, uint16_t &outLen) {
    // Mark as dictionary-compressed
    out[0] = 0xD0;  // Dict mode marker
    uint16_t oIdx = 1;
    uint16_t iIdx = 0;

    while (iIdx < inLen) {
        bool matched = false;

        // Try to match dictionary entries
        for (uint8_t d = 0; d < dictSize; d++) {
            uint8_t wordLen = strlen(dictionary[d].word);
            if (iIdx + wordLen <= inLen) {
                // Case-insensitive compare
                bool match = true;
                for (uint8_t c = 0; c < wordLen; c++) {
                    char ic = in[iIdx + c];
                    if (ic >= 'a' && ic <= 'z') ic -= 32;
                    if (ic != dictionary[d].word[c]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    out[oIdx++] = 0xFF;  // Escape: next byte is dict code
                    out[oIdx++] = dictionary[d].code;
                    iIdx += wordLen;
                    matched = true;
                    break;
                }
            }
        }

        if (!matched) {
            if (in[iIdx] == 0xFF) {
                // Escape literal 0xFF
                out[oIdx++] = 0xFF;
                out[oIdx++] = 0xFF;
            } else {
                out[oIdx++] = in[iIdx];
            }
            iIdx++;
        }
    }

    outLen = oIdx;
    return 0;
}

int MeshXTCompress::dictDecompress(const uint8_t *in, uint16_t inLen,
                                    uint8_t *out, uint16_t &outLen) {
    if (inLen < 1 || in[0] != 0xD0) return -1;  // Not dict-compressed

    uint16_t iIdx = 1;
    uint16_t oIdx = 0;

    while (iIdx < inLen) {
        if (in[iIdx] == 0xFF && iIdx + 1 < inLen) {
            uint8_t code = in[iIdx + 1];
            if (code == 0xFF) {
                out[oIdx++] = 0xFF;
            } else {
                // Look up dictionary
                bool found = false;
                for (uint8_t d = 0; d < dictSize; d++) {
                    if (dictionary[d].code == code) {
                        uint8_t wl = strlen(dictionary[d].word);
                        memcpy(out + oIdx, dictionary[d].word, wl);
                        oIdx += wl;
                        found = true;
                        break;
                    }
                }
                if (!found) out[oIdx++] = '?';
            }
            iIdx += 2;
        } else {
            out[oIdx++] = in[iIdx++];
        }
    }

    outLen = oIdx;
    return 0;
}

int MeshXTCompress::rleCompress(const uint8_t *in, uint16_t inLen,
                                 uint8_t *out, uint16_t &outLen) {
    out[0] = 0xE0;  // RLE mode marker
    uint16_t oIdx = 1;

    for (uint16_t i = 0; i < inLen; ) {
        uint8_t ch = in[i];
        uint8_t count = 1;
        while (i + count < inLen && in[i + count] == ch && count < 255) {
            count++;
        }

        if (count >= 3) {
            out[oIdx++] = 0xFE;  // RLE marker
            out[oIdx++] = count;
            out[oIdx++] = ch;
        } else {
            for (uint8_t j = 0; j < count; j++) {
                out[oIdx++] = ch;
            }
        }
        i += count;
    }

    outLen = oIdx;
    return 0;
}

int MeshXTCompress::rleDecompress(const uint8_t *in, uint16_t inLen,
                                   uint8_t *out, uint16_t &outLen) {
    if (inLen < 1 || in[0] != 0xE0) return -1;

    uint16_t iIdx = 1;
    uint16_t oIdx = 0;

    while (iIdx < inLen) {
        if (in[iIdx] == 0xFE && iIdx + 2 < inLen) {
            uint8_t count = in[iIdx + 1];
            uint8_t ch    = in[iIdx + 2];
            for (uint8_t j = 0; j < count; j++) {
                out[oIdx++] = ch;
            }
            iIdx += 3;
        } else {
            out[oIdx++] = in[iIdx++];
        }
    }

    outLen = oIdx;
    return 0;
}
