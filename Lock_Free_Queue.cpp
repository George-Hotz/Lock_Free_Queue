#include <atomic>
#include <memory>
#include <iostream>

// Node结构体定义，用于存储队列中的元素
template<typename T>
struct Node {
    std::shared_ptr<T> data;
    std::atomic<Node*> next;

    Node(T value) : data(std::make_shared<T>(value)), next(nullptr) {}
};

// 无锁队列类
template<typename T>
class LockFreeQueue {
private:
    std::atomic<Node<T>*> head;
    std::atomic<Node<T>*> tail;

public:
    // 构造函数：初始化队列的头和尾节点，初始状态为空节点
    LockFreeQueue() {
        Node<T>* dummy = new Node<T>(T{});  // 创建一个哨兵节点
        head.store(dummy);
        tail.store(dummy);
    }

    // 入队操作
    void enqueue(T value) {
        Node<T>* newNode = new Node<T>(value);  // 为新元素创建节点
        Node<T>* oldTail;
        while (true) {
            oldTail = tail.load();  // 获取当前的尾节点
            Node<T>* next = oldTail->next.load();
            if (oldTail == tail.load()) {  // 检查是否有其他线程修改了尾节点
                if (next == nullptr) {  // 如果next为空，表示尾节点是最新的
                    if (oldTail->next.compare_exchange_weak(next, newNode)) {  // 尝试将next设置为新节点
                        break;  // 设置成功，退出循环
                    }
                } else {
                    // 如果tail不在队列的最后，尝试帮助推进tail
                    tail.compare_exchange_weak(oldTail, next);
                }
            }
        }
        // 尝试更新尾指针到新节点
        tail.compare_exchange_weak(oldTail, newNode);
    }

    // 出队操作
    std::shared_ptr<T> dequeue() {
        Node<T>* oldHead;
        while (true) {
            oldHead = head.load();  // 获取当前的头节点
            Node<T>* oldTail = tail.load();
            Node<T>* next = oldHead->next.load();
            if (oldHead == head.load()) {  // 检查头节点是否被其他线程修改
                if (oldHead == oldTail) {  // 队列为空或正处于插入新节点的过程中
                    if (next == nullptr) {
                        return std::shared_ptr<T>();  // 队列为空，返回空指针
                    }
                    // 尝试帮助推进tail
                    tail.compare_exchange_weak(oldTail, next);
                } else {
                    // 尝试更新head指针到next节点
                    if (head.compare_exchange_weak(oldHead, next)) {
                        std::shared_ptr<T> res = oldHead->next.load()->data;  // 获取数据
                        delete oldHead;  // 释放旧的头节点
                        return res;  // 返回出队的数据
                    }
                }
            }
        }
    }

    // 析构函数：清理节点
    ~LockFreeQueue() {
        while (dequeue()) {}  // 清空队列中的所有元素
        delete head.load();  // 删除哨兵节点
    }
};

// 测试无锁队列
int main() {
    LockFreeQueue<int> queue;

    // 测试入队
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);

    // 测试出队
    std::cout << "Dequeued: " << *queue.dequeue() << std::endl;  // 输出 1
    std::cout << "Dequeued: " << *queue.dequeue() << std::endl;  // 输出 2
    std::cout << "Dequeued: " << *queue.dequeue() << std::endl;  // 输出 3

    // 测试队列为空时出队
    if (queue.dequeue() == nullptr) {
        std::cout << "Queue is empty!" << std::endl;
    }

    return 0;
}
