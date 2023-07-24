# tiny-prof

> 性能优化的第一步是 profiling

![火焰图](image.png)

## 为什么不用 gprof ?

1. 不支持多线程
1. 没有好看的界面
1. K230 rt-smart 工具链不支持 gprof
1. 线程挂起时不记录
1. 对动态库支持不好

## 使用方法

1. 编译 `hook.c` 到 `hook.o`，可以使用 `-O3` 优化
1. 编译自己的代码，添加 `-finstrument-functions` 参数，建议添加 `-g` 生成调试符号，避免在最后只能看到一堆函数指针
1. 链接可执行程序时把 `hook.o` 一起链接进去
1. 运行程序，如果在开始时打印了 `start profiling...` 则表示 tiny-prof 成功添加到程序中，程序正常退出时会打印 `end profiling, xxx records, checkout a.profile`，xxx表示记录的函数调用次数，同时生成 `a.profile` 文件
1. 打开 [tiny-prof-viewer]()
1. 点击右侧的 `Add symbols` 选择运行的可执行 ELF 文件，添加完成后下方的 Symbols 会显示加载了多少个符号
1. 点击右侧的 `Open profile` 选择生成的 `a.profile` 文件，等待查看请加载

## 示例

项目自带了两个示例，`test` 和 `test_cpp`，均为多线程，直接 `make` 即可编译，运行后都会生成 profile 文件

## Roadmap

- [ ] Statistics infomation
- [ ] Hide unselected threads
- [ ] Shared library symbols
- [ ] Optimize flame chart performance
- [ ] Color