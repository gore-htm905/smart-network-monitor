#ifndef CIRCULARLINKEDLIST_H
#define CIRCULARLINKEDLIST_H

#include <string>
#include <vector>

struct CLLNode {
    std::string nodeId;
    bool isActive;
    CLLNode* next;

    CLLNode(const std::string& id) : nodeId(id), isActive(true), next(nullptr) {}
};

// Circular Linked List for Round-Robin Node Scheduling
class CircularLinkedList {
private:
    CLLNode* tail;
    CLLNode* current;
    int count;

public:
    CircularLinkedList() : tail(nullptr), current(nullptr), count(0) {}

    ~CircularLinkedList() {
        if (tail == nullptr) return;
        CLLNode* curr = tail->next;
        while (curr != tail) {
            CLLNode* next = curr->next;
            delete curr;
            curr = next;
        }
        delete tail;
    }

    void addNode(const std::string& nodeId) {
        CLLNode* newNode = new CLLNode(nodeId);
        if (tail == nullptr) {
            tail = newNode;
            tail->next = tail;
            current = tail;
        } else {
            newNode->next = tail->next;
            tail->next = newNode;
            tail = newNode;
        }
        count++;
    }
    
    void setNodeStatus(const std::string& nodeId, bool active) {
        if (tail == nullptr) return;
        CLLNode* curr = tail->next;
        do {
            if (curr->nodeId == nodeId) {
                curr->isActive = active;
                break;
            }
            curr = curr->next;
        } while (curr != tail->next);
    }

    // Get the next active node in the round-robin schedule
    std::string getNextActiveNode() {
        if (count == 0 || current == nullptr) return "";

        CLLNode* start = current;
        do {
            current = current->next;
            if (current->isActive) {
                return current->nodeId;
            }
        } while (current != start);

        return ""; // No active nodes found
    }
    
    // For visualization
    std::vector<std::pair<std::string, bool>> getNodes() const {
        std::vector<std::pair<std::string, bool>> nodes;
        if (tail == nullptr) return nodes;
        
        CLLNode* curr = tail->next;
        do {
            nodes.push_back({curr->nodeId, curr->isActive});
            curr = curr->next;
        } while (curr != tail->next);
        
        return nodes;
    }
};

#endif // CIRCULARLINKEDLIST_H
