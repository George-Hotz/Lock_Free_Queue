# 无锁队列
Michael-Scott无锁队列依赖于原子操作和Compare-And-Swap(CAS)来保证线程安全，有两种实现方式，

- 🚀基于数组的无锁队列（通常是环形队列）
- 🚀基于链表的无锁队列

在比较基于链表和基于数组的无锁队列性能时，关键的性能指标主要包括：
 - 1、内存分配与访问效率
 - 2、缓存局部性
 - 3、操作复杂度（尤其是 enqueue 和 dequeue 的操作）
 - 4、扩展性与容量限制
 - 5、内存管理开销

![list_array_compare](./assets/list_array_compare.png)

# 适用场景
 - 基于链表的无锁队列💡：适用于需要动态调整队列大小、并且元素数量无法预估的场景。比如不确定队列最大长度的任务调度或请求处理系统。
 - 基于数组的无锁队列💡：适用于已知最大容量、对性能要求较高、内存分配频率较低的场景。比如实时系统中的生产者-消费者模型或固定缓冲区的网络包处理系统。
