# 第三方代码声明

## Micro LZMA decoder for x86 (static)

- 原作者：Ilya Kurdyukov
- 原始版权：Copyright (c) 2022, Ilya Kurdyukov
- 原始源码：[micro-lzmadec/static.x86.asm](https://github.com/ilyakurdyukov/micro-lzmadec/blob/main/static.x86.asm)
- 许可：[Creative Commons Attribution 3.0 (CC BY 3.0)](https://creativecommons.org/licenses/by/3.0/)
- 本项目中的派生文件：`SRC/lzma_static_x86.asm`

本项目保留了原文件中的版权、许可和免责声明。本项目对原始解码器进行了适配和修改，包括汇编调用接口、按输出长度停止、概率表与解码状态初始化，以及与 fast 7z C 入口的集成。当前文件不是未经修改的上游 480 字节版本。

该署名和 CC BY 3.0 许可说明适用于上述派生汇编代码及其原始部分；它不单独规定本项目其他文件的许可。
