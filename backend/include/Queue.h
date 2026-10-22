#ifndef QUEUE_H
#define QUEUE_H

#include "Packet.h"
#include <stdexcept>
#include <vector>

// Priority-aware Queue for router and node processing
class Queue {
private:
    std::vector<Packet> data;
    int capacity;
    bool priorityOrder;

public:
    Queue(int cap = 20, bool priority = true) : capacity(cap), priorityOrder(priority) {
        data.reserve(capacity);
    }

    bool enqueue(const Packet& p) {
        if (isFull()) return false;
        if (!priorityOrder) {
            data.push_back(p);
            return true;
        }
        // Priority ordering: HIGH (2) > MEDIUM (1) > LOW (0)
        // Stable within same priority level (preserves FIFO arrival order)
        auto it = data.begin();
        while (it != data.end() && it->priority >= p.priority) {
            ++it;
        }
        data.insert(it, p);
        return true;
    }

    Packet dequeue() {
        if (isEmpty()) throw std::out_of_range("Queue is empty");
        Packet p = data.front();
        data.erase(data.begin());
        return p;
    }

    Packet peek() const {
        if (isEmpty()) throw std::out_of_range("Queue is empty");
        return data.front();
    }

    bool isEmpty() const { return data.empty(); }
    bool isFull() const { return (int)data.size() >= capacity; }
    int size() const { return (int)data.size(); }
    int getCapacity() const { return capacity; }
    
    // Drain all packets from the queue and return them. Queue is empty afterwards.
    std::vector<Packet> drain() {
        std::vector<Packet> out;
        out.swap(data);
        return out;
    }

    // For visualization
    std::vector<Packet> getElements() const {
        return data;
    }
};

#endif // QUEUE_H
