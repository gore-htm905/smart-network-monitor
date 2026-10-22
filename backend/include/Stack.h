#ifndef STACK_H
#define STACK_H

#include <vector>
#include <string>
#include <stdexcept>

// Stack for packet route history
class Stack {
private:
    std::vector<std::string> elements;
    
public:
    void push(const std::string& node) {
        elements.push_back(node);
    }
    
    std::string pop() {
        if (isEmpty()) throw std::out_of_range("Stack is empty");
        std::string top = elements.back();
        elements.pop_back();
        return top;
    }
    
    std::string peek() const {
        if (isEmpty()) throw std::out_of_range("Stack is empty");
        return elements.back();
    }
    
    bool isEmpty() const {
        return elements.empty();
    }
    
    std::vector<std::string> getElements() const {
        // Return elements from top to bottom (reverse of insertion order)
        std::vector<std::string> reversed;
        for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
            reversed.push_back(*it);
        }
        return reversed;
    }
};

#endif // STACK_H
