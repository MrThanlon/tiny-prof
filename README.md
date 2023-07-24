# tiny-prof

> 性能优化的第一步是 profiling

![火焰图](image.png)

## 为什么不用 gprof ?

1. 不支持多线程
1. 没有好看的界面
1. K230 rt-smart 工具链不支持 gprof
1. 线程挂起时不记录
1. 对动态库支持不好

## 原理

GCC 使用 `-finstrument-functions` 可以对源码进行函数级打桩，在每个函数的开始前调用 `__cyg_profile_func_enter()`，返回前调用 `__cyg_profile_func_exit`，只要编写这两个函数就可以记录被打桩函数的运行时间。

## 使用方法

1. 编译 `hook.c` 到 `hook.o`，可以使用 `-O3` 优化
1. 编译自己的代码，添加 `-finstrument-functions` 参数，建议添加 `-g` 生成调试符号，避免在最后只能看到一堆函数指针
1. 链接可执行程序时把 `hook.o` 一起链接进去
1. 运行程序，如果在开始时打印了 `start profiling...` 则表示 tiny-prof 成功添加到程序中，程序正常退出时会打印 `end profiling, xxx records, checkout a.profile`，xxx表示记录的函数调用次数，同时生成 `a.profile` 文件
1. 打开 [tiny-prof-viewer](https://ai.b-bug.org/~huangziyi/tiny-prof-viewer/)
1. 点击右侧的 `Add symbols` 选择运行的可执行 ELF 文件，添加完成后下方的 Symbols 会显示加载了多少个符号
1. 点击右侧的 `Open profile` 选择生成的 `a.profile` 文件，等待查看请加载

### 一些建议

一般来说，火焰图中的大平顶表示这个函数执行的时间比较多，比如存在一个耗时较长的循环，这样的函数可以重点优化，而火焰图中的尖峰通常表示这里的函数调用较为密集，能够优化的空间可能较小。

## 示例

项目自带了两个示例，`test` 和 `test_cpp`，均为多线程，直接 `make` 即可编译，运行后都会生成 profile 文件

## Roadmap

- [ ] Statistics infomation
- [ ] Hide unselected threads
- [ ] Shared library symbols
- [ ] Optimize flame chart performance
- [ ] Color
