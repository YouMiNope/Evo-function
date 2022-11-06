# Evo-function
通过随机突变机器码来进化目标函数

## Build
除了Linux可能都跑不了（mac上不知怎么进程数量容易超上限）

目前（22年10月25日）有三个文件 main view 与 test

1. 使用make main编译 main
2. 使用make view编译 view
3. 使用make test编译 test

为什么不直接make编译全部呢？因为我还不会写makefile（

## Run

### 运行main

    ./main ./targetfunc.o
    或者直接 make run
    
这个过程会创建一块共享内存。如需释放请手动ipcrm -m (your_shm_id)

### 监视进化状态

    ./view
    
就可以看到一个简单的界面...
按下esc退出后就能保存进化的那些个函数了

### 测试函数

    ./test ./save/(some_id)

非常简陋的测试模块，以后再写






