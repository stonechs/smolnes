#ifndef SEVENZIP_SINGLE_H
#define SEVENZIP_SINGLE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SevenZipResult {
    SEVENZIP_OK = 0,
    SEVENZIP_INVALID_ARGUMENT,
    SEVENZIP_INVALID_ARCHIVE,
    SEVENZIP_UNSUPPORTED,
    SEVENZIP_OUTPUT_TOO_SMALL,
    SEVENZIP_MEMORY_ERROR,
    SEVENZIP_DECODE_ERROR,
    SEVENZIP_CRC_ERROR
} SevenZipResult;

/*
 * 唯一的归档入口：按固定字段顺序读取解码所需信息并调用 fast decoder，
 * 不验证归档结构或 CRC。
 * archive/archive_size 是完整 7z 文件的只读地址和字节数；output/
 * output_capacity 是调用方提供的可写解压缓冲区和容量；output_size 必须指向
 * 有效的 size_t，成功时接收实际输出长度。此入口不验证指针、归档长度或容量。
 * 仅适用于完整可信的 7z 文件：普通 Header 后紧跟 MainStreamsInfo；一个非空
 * 文件、一个 packed stream、一个 LZMA1 coder，coder ID 为 3 字节且属性为 5 字节。
 * workspace 由 C 端按最大概率表容量分配；汇编入口将每项初始化为 1024，LZMA
 * 状态和重复距离也在汇编调用栈上初始化。汇编入口接收 src、dst、workspace、
 * output_size 和 lc/lp/pb。
 * 不支持 ArchiveProperties、AdditionalStreamsInfo、压缩 Header、加密、滤镜、
 * 多文件、多流或分卷。调用方必须确保输出容量足够，解码输入完整且属性正确。
 * fast-only 静态库的 32 位变长整数读取器要求所有长度和偏移小于 100 MiB，
 * 并要求每个变长整数位置之后至少有 4 个可读字节。
 */
SevenZipResult sevenzip_extract_single_lzma1_fast(
    const uint8_t *archive, size_t archive_size,
    uint8_t *output, size_t output_capacity, size_t *output_size);

#ifdef __cplusplus
}
#endif
#endif
