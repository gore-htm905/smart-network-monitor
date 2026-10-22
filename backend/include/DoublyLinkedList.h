#ifndef DOUBLYLINKEDLIST_H
#define DOUBLYLINKEDLIST_H

#include "Packet.h"
#include <vector>

struct DLLNode {
    Packet data;
    DLLNode* prev;
    DLLNode* next;

    DLLNode(const Packet& p) : data(p), prev(nullptr), next(nullptr) {}
};

// Doubly Linked List for Packet History
class DoublyLinkedList {
private:
    DLLNode* head;
    DLLNode* tail;
    int count;

public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}

    ~DoublyLinkedList() {
        DLLNode* current = head;
        while (current != nullptr) {
            DLLNode* next = current->next;
            delete current;
            current = next;
        }
    }

    void append(const Packet& p) {
        DLLNode* newNode = new DLLNode(p);
        if (head == nullptr) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        count++;
    }

    std::vector<Packet> getHistory() const {
        std::vector<Packet> history;
        DLLNode* current = head;
        while (current != nullptr) {
            history.push_back(current->data);
            current = current->next;
        }
        return history;
    }
    
    // Allows backwards traversal
    std::vector<Packet> getHistoryReverse() const {
        std::vector<Packet> history;
        DLLNode* current = tail;
        while (current != nullptr) {
            history.push_back(current->data);
            current = current->prev;
        }
        return history;
    }

    int size() const {
        return count;
    }
};

#endif // DOUBLYLINKEDLIST_H
