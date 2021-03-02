#ifndef SAFEQUEUE
#define SAFEQUEUE

#include <queue>
#include<mutex>

template <class T>
class SafeQueue {
private:
	std::queue<T> q;
	mutable std::mutex m;
	std::condition_variable c;

public:
	SafeQueue() = default;
	~SafeQueue() {};

	void push(T a) {
		std::lock_guard<std::mutex> lock(m);
		q.push(a);
		c.notify_one();
	}

	T pop() {
		std::unique_lock <std::mutex> lock(m);
		while (q.empty()) {
			c.wait(lock);
		}
		T val = q.front();
		q.pop();

		return val
	}
};

#endif
