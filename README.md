# ProgramServer4
**暂未进行压力测试**
1. 目前一个线程能够管理的线程数为3，若要进行压力测试，需修改SOCK_QUEUE_SIZE的值，其表示一个select模型管理的套接字数目
2. 存在当所有Client关闭后，Server闪退的问题
3. 服务器现在收了就发，暂未对send<=0进行异常处理，压力测试的时候100%出问题
