#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity) 
        : capacity_(capacity), head_(0), tail_(0) {
        buffer_ = new std::atomic<T*>[capacity];
        for (size_t i = 0; i < capacity; ++i) {
            buffer_[i].store(nullptr, std::memory_order_relaxed);
        }
    }

    ~LockFreeQueue() {
        for (size_t i = 0; i < capacity_; ++i) {
            T* item = buffer_[i].load();
            if (item != nullptr) {
                delete item;
            }
        }
        delete[] buffer_;
    }

    // 入队操作
    bool enqueue(const T& value) {
        T* newValue = new T(value);  // 动态分配新值，避免并发冲突

        while (true) {
            size_t tail = tail_.load(std::memory_order_relaxed);
            size_t nextTail = (tail + 1) % capacity_;
            if (nextTail == head_.load(std::memory_order_acquire)) {
                // 队列已满
                delete newValue;
                return false;
            }

            // 尝试将值放入队列尾部
            T* expected = nullptr;
            if (buffer_[tail].compare_exchange_weak(expected, newValue, std::memory_order_release, std::memory_order_relaxed)) {
                // 成功入队，移动尾指针
                tail_.store(nextTail, std::memory_order_release);
                return true;
            }
        }
    }

    // 出队操作
    bool dequeue(T& result) {
        while (true) {
            size_t head = head_.load(std::memory_order_relaxed);
            if (head == tail_.load(std::memory_order_acquire)) {
                // 队列为空
                return false;
            }

            // 尝试从队列头部取出元素
            T* value = buffer_[head].load(std::memory_order_acquire);
            if (value && buffer_[head].compare_exchange_weak(value, nullptr, std::memory_order_release, std::memory_order_relaxed)) {
                // 成功出队，移动头指针
                result = *value;
                delete value;  // 释放内存
                head_.store((head + 1) % capacity_, std::memory_order_release);
                return true;
            }
        }
    }

private:
    std::atomic<T*>* buffer_;  // 环形缓冲区
    const size_t capacity_;    // 缓冲区容量
    std::atomic<size_t> head_; // 队列头指针
    std::atomic<size_t> tail_; // 队列尾指针
};

int main() {
    LockFreeQueue<int> queue(5);

    // 启动生产者线程
    std::thread producer([&queue]() {
        for (int i = 0; i < 10; ++i) {
            while (!queue.enqueue(i)) {
                std::this_thread::yield();  // 队列满时主动让出 CPU
            }
            std::cout << "Produced: " << i << std::endl;
        }
    });

    // 启动消费者线程
    std::thread consumer([&queue]() {
        int value;
        for (int i = 0; i < 10; ++i) {
            while (!queue.dequeue(value)) {
                std::this_thread::yield();  // 队列空时主动让出 CPU
            }
            std::cout << "Consumed: " << value << std::endl;
        }
    });

    producer.join();
    consumer.join();

    return 0;
}
