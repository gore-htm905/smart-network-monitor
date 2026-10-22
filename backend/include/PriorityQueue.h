#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include "Packet.h"
#include <vector>
#include <stdexcept>
#include <algorithm>

// Priority Queue for scheduling packets (Max Heap based on priority)
class PriorityQueue {
private:
    std::vector<Packet> heap;

    int parent(int i) { return (i - 1) / 2; }
    int leftChild(int i) { return 2 * i + 1; }
    int rightChild(int i) { return 2 * i + 2; }

    void heapifyUp(int i) {
        while (i != 0 && heap[parent(i)].priority < heap[i].priority) {
            std::swap(heap[i], heap[parent(i)]);
            i = parent(i);
        }
    }

    void heapifyDown(int i) {
        int largest = i;
        int left = leftChild(i);
        int right = rightChild(i);
        int n = heap.size();

        if (left < n && heap[left].priority > heap[largest].priority) {
            largest = left;
        }

        if (right < n && heap[right].priority > heap[largest].priority) {
            largest = right;
        }

        if (largest != i) {
            std::swap(heap[i], heap[largest]);
            heapifyDown(largest);
        }
    }

public:
    void enqueue(const Packet& p) {
        heap.push_back(p);
        heapifyUp(heap.size() - 1);
    }

    Packet dequeue() {
        if (isEmpty()) throw std::out_of_range("Priority Queue is empty");
        Packet root = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        heapifyDown(0);
        return root;
    }

    Packet peek() const {
        if (isEmpty()) throw std::out_of_range("Priority Queue is empty");
        return heap[0];
    }

    bool isEmpty() const {
        return heap.empty();
    }

    int size() const {
        return heap.size();
    }
    
    // Drain all packets and return them. Heap is empty afterwards.
    std::vector<Packet> drainAll() {
        std::vector<Packet> out;
        out.swap(heap);
        return out;
    }

    // For visualization on frontend
    std::vector<Packet> getElements() const {
        return heap;
    }
};

#endif // PRIORITYQUEUE_H
