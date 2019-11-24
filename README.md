# 模拟的情况是data chunk和parity chunk分开存放，并且节点平均分配给每个机架，后面将存data chunk的叫data rack，存parity chunk的叫parity rack。
代码中N和K表示使用的MDS码的参数是（N，K），M表示data rack的数量，P表示parity rack的数量，因为是平均分配，所以需要保证K可以整除M，N-K可以整除P。
