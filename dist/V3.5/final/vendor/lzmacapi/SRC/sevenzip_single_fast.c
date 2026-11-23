#include "sevenzip_single.h"

#include <stdlib.h>
#include <string.h>

#define SEVENZIP_START_SIZE 32u
#define SEVENZIP_MAX_DECODED 0x01000000u
#define LZMA1_MAX_PROB_COUNT (2048u + (768u << 12))

typedef struct FastFolder {
    size_t packed_offset;
    size_t packed_size;
    size_t unpacked_size;
    uint8_t properties[5];
} FastFolder;

static uint64_t read_u64le(const uint8_t *p)
{
    uint64_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

/*
 * 7z 变长整数的精简读取器：一次读取 4 字节，根据首字节前导位数取值。
 * fast API 的约束是所有长度/偏移小于 100 MiB（至多编码 4 字节），且
 * 归档在该字段之后至少还有 4 个可读字节；因此可省去通用 UINT64 解码逻辑。
 */
static uint32_t read_number_fast(const uint8_t **cursor)
{
    uint32_t raw;
    unsigned first;
    memcpy(&raw, *cursor, sizeof(raw));
    first = raw & 0xFFu;
    if (!(first & 0x80u)) {
        ++*cursor;
        return first;
    }
    if (!(first & 0x40u)) {
        *cursor += 2;
        return ((raw >> 8) & 0xFFu) | ((first & 0x3Fu) << 8);
    }
    if (!(first & 0x20u)) {
        *cursor += 3;
        return ((raw >> 8) & 0xFFFFu) | ((first & 0x1Fu) << 16);
    }
    *cursor += 4;
    return ((raw >> 8) & 0xFFFFFFu) | ((first & 0x0Fu) << 24);
}

/* 将 LZMA1 属性字节拆成当前解码核心需要的 lc、lp、pb。 */
static int decode_properties(const uint8_t *props, unsigned *lc,
                             unsigned *lp, unsigned *pb)
{
    unsigned value = props[0];
#ifdef LZMA_C_API_DEBUG
    if (value >= 225) return 0;
#endif
    *lc = value % 9;
    *lp = (value / 9) % 5;
    *pb = value / 45;
#ifdef LZMA_C_API_DEBUG
    return *lc <= 8 && *lp <= 4 && *lc + *lp <= 12 && *pb <= 4;
#else
    return 1;
#endif
}

/* fast-only 静态库自己的 folder 解码实现，不依赖完整 7z 解析器。 */
static SevenZipResult decode_folder(const uint8_t *archive, size_t archive_size,
                                    size_t packed_limit, const FastFolder *folder,
                                    uint8_t *output, size_t output_capacity,
                                    size_t *output_size)
{
    unsigned lc, lp, pb;
    size_t workspace_size;
    uint16_t *workspace;
#ifdef LZMA_C_API_DEBUG
    uint8_t *end;
#endif
#ifdef LZMA_C_API_DEBUG
    if (folder->packed_offset > packed_limit ||
        folder->packed_size > packed_limit - folder->packed_offset ||
        folder->packed_offset > archive_size ||
        folder->packed_size > archive_size - folder->packed_offset ||
        folder->packed_size < 5 || folder->unpacked_size > SEVENZIP_MAX_DECODED)
        return SEVENZIP_INVALID_ARCHIVE;
    if (folder->unpacked_size > output_capacity ||
        (folder->unpacked_size && !output))
        return SEVENZIP_OUTPUT_TOO_SMALL;
#else
    (void)archive_size;
    (void)packed_limit;
    (void)output_capacity;
#endif
    if (!decode_properties(folder->properties, &lc, &lp, &pb))
        return SEVENZIP_UNSUPPORTED;
    workspace_size = (size_t)LZMA1_MAX_PROB_COUNT * sizeof(*workspace);
    workspace = (uint16_t *)malloc(workspace_size);
    if (!workspace) return SEVENZIP_MEMORY_ERROR;
#ifdef LZMA_C_API_DEBUG
    if (archive[folder->packed_offset] != 0) {
        free(workspace);
        return SEVENZIP_DECODE_ERROR;
    }
    end = lzma_static_x86_run(archive + folder->packed_offset, output, workspace,
                              folder->unpacked_size, lc, lp, pb);
    free(workspace);
    if (end < output || (size_t)(end - output) != folder->unpacked_size)
        return SEVENZIP_DECODE_ERROR;
#else
    lzma_static_x86_run(archive + folder->packed_offset, output, workspace,
                        folder->unpacked_size, lc, lp, pb);
    free(workspace);
#endif
    *output_size = folder->unpacked_size;
    return SEVENZIP_OK;
}

/*
 * 快速路径：按固定布局提取 LZMA1 解码所需字段，然后由本文件中的
 * decode_folder 直接调用汇编解码核心。它不验证 7z 结构字段或 CRC。
 *
 * 参数：archive 是完整 7z 文件的只读地址，archive_size 是字节数；output
 * 是调用方提供的可写解压缓冲区，output_capacity 是其容量；output_size 指向
 * 有效的 size_t，成功时接收输出长度。发布构建不验证参数、输入边界和输出容量。
 *
 * 输入约束：可信且完整的普通 Header 单文件归档；MainStreamsInfo 紧跟 Header，
 * 仅有一个非空文件、一个 packed stream 和一个 LZMA1 coder。所有 7z 长度及偏移
 * 小于 100 MiB，且每个变长整数位置至少有 4 个可读字节。调用方保证输出容量足够。
 */
SevenZipResult sevenzip_extract_single_lzma1_fast(
    const uint8_t *archive, size_t archive_size,
    uint8_t *output, size_t output_capacity, size_t *output_size)
{
    size_t next_offset, next_size;
    const uint8_t *cursor;
    uint8_t tag, all_digests;
    FastFolder folder;

    *output_size = 0;
    next_offset = SEVENZIP_START_SIZE + (size_t)read_u64le(archive + 12);
    next_size = (size_t)read_u64le(archive + 20);
    cursor = archive + next_offset;

    /* Header、MainStreamsInfo、PackInfo、PackPos、packed stream size。 */
    cursor += 3;
    folder.packed_offset = SEVENZIP_START_SIZE + read_number_fast(&cursor);
    cursor += 2;
    folder.packed_size = read_number_fast(&cursor);

    /* Pack CRC 可选字段只跳过，不计算或比较。 */
    tag = *cursor++;
    if (tag == 10) {
        all_digests = *cursor++;
        if (all_digests) {
            cursor += 4;
        } else if (*cursor++ & 0x80u) {
            cursor += 4;
        }
        ++cursor;
    }

    /* 固定布局中的 UnpackInfo、folder、coder 和 LZMA1 方法 ID。 */
    cursor += 9;
    ++cursor; /* 属性长度字段固定为 5。 */
    folder.properties[0] = *cursor;
    cursor += sizeof(folder.properties);
    ++cursor; /* UnpackSize tag。 */
    folder.unpacked_size = read_number_fast(&cursor);
    return decode_folder(archive, archive_size, next_offset, &folder, output,
                         output_capacity, output_size);
}
