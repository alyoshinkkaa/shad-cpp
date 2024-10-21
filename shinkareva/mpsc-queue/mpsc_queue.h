#include <atomic>

template <class T>
class MPSCQueue {
private:
    struct Node {
        T value;
        Node* next;
        Node(const T& val) : value(val), next(nullptr) {
        }
    };

    std::atomic<Node*> head_;

public:
    MPSCQueue() : head_(nullptr) {
    }

    void Push(const T& value) {
        Node* new_node = new Node(value);
        new_node->next = head_.load();
        while (!head_.compare_exchange_weak(new_node->next, new_node)) {
        }
    }

    std::pair<T, bool> Pop() {
        Node* old_head = head_.load();
        if (!old_head) {
            return std::make_pair(T(), false);
        }
        Node* new_head = old_head->next;
        while (!head_.compare_exchange_weak(old_head, new_head)) {
            if (!old_head) {
                return std::make_pair(T(), false);
            }
            new_head = old_head->next;
        }
        T value = old_head->value;
        delete old_head;
        return std::make_pair(value, true);
    }

    void DequeueAll(auto&& callback) {
        Node* current = head_.exchange(nullptr);
        while (current) {
            Node* next = current->next;
            callback(current->value);
            delete current;
            current = next;
        }
    }

    ~MPSCQueue() {
        Node* current = head_.exchange(nullptr);
        while (current) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }
};