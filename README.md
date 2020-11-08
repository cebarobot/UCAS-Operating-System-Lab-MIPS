# UCAS OS Project MIPS
## Project 1: Bootloader
编译步骤：
```shell
$ cd test/test_project1/1-2
$ make
```
编译前可能需要修改 `test/test_project1/1-2/Makefile` 以指定输出镜像地址。

## Project 2: A Simple Kernel
编译步骤：
```shell
$ cd test/test_project2/
$ make
```
编译得到的 `disk` 文件位于仓库主目录下，名为 `qemu-disk`。

## Other Feature Note
* 目前版本中并没有使用 `$gp`（global pointer）寄存器进行全局变量的寻址。
  * 在 `Makefile` 中，目前使用了 `-G 0` 参数，表示大小小于 0 字节的变量才会使用 `$gp` 寻址（等同于禁用）。
  * 初始化 `$gp` 可参考：[How to properly initialize $gp on mips?](https://users.rust-lang.org/t/how-to-properly-initialize-gp-on-mips/30508)（但这个是 Rust 的……）
* `.equ` 和 `#define` 的区别是什么？
* 目前，中断处理是把中断类型判断放到了 C 语言中的 `interrupt_helper` 函数，这会导致性能下降。需要将这一部分修改为汇编。