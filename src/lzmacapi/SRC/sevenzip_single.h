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
 * 裸 LZMA1 解码入口：不解析 7z 或 LZMA-Alone 容器，只解码 range-coded 数据。
 * src 指向 5 字节 range 初始化前缀的首字节：src[0] 必须为 0，src[1..4]
 * 是初始 Code，后面紧跟压缩数据。解码停止位置为 dst + output_size。
 * workspace 指向至少 6,295,552 字节、按 uint16_t 对齐的可写内存；汇编每次
 * 调用都会将最大概率表初始化为 1024。lc/lp/pb 必须是码流对应的有效属性。
 * 此入口没有输入长度和输出容量参数，也不验证码流；调用方必须保证输入完整、
 * 输出容量足够、src/dst/workspace 不相互覆盖，并传入正确的输出长度。
 * 发布库返回 void；定义 LZMA_C_API_DEBUG 并链接调试库时返回实际输出末地址，
 * 调用方必须让头文件宏与所链接的库保持一致。
 */
#ifdef LZMA_C_API_DEBUG
uint8_t *__cdecl lzma_static_x86_run(
    const uint8_t *src, uint8_t *dst, void *workspace,
    size_t output_size, unsigned lc, unsigned lp, unsigned pb);
#else
void __cdecl lzma_static_x86_run(
    const uint8_t *src, uint8_t *dst, void *workspace,
    size_t output_size, unsigned lc, unsigned lp, unsigned pb);
#endif

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
