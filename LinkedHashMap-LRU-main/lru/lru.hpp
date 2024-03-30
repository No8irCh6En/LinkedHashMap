#ifndef SJTU_LRU_HPP
#define SJTU_LRU_HPP
#define DEBUG
#include "class-integer.hpp"
#include "class-matrix.hpp"
#include "exceptions.hpp"
#include "utility.hpp"
#define CAPACITY_DEFAULT 16
class Hash {
   public:
    unsigned int operator()(Integer lhs) const {
        int val = lhs.val;
        return std::hash<int>()(val);
    }
};
class Equal {
   public:
    bool operator()(const Integer& lhs, const Integer& rhs) const {
        return lhs.val == rhs.val;
    }
};

namespace sjtu {
template <class T>
class double_list {
   public:
    class Node {
       public:
        T* val_ptr;
        Node* prev;
        Node* next;
        Node* dual;
        Node(T* val_ptr = nullptr,
             Node* prev = nullptr,
             Node* next = nullptr,
             Node* dual = nullptr)
            : val_ptr(val_ptr), prev(prev), next(next), dual(dual) {}
        ~Node() {
            if (dual) {
                dual->dual = nullptr;
            }
            dual = nullptr;
            if (val_ptr)
                delete val_ptr;
        }
    };
    Node* end_ptr;
    Node end_node;
    Node* head;
    size_t size;
    // --------------------------

    double_list() : size(0), end_node() { head = end_ptr = &end_node; }
    double_list(const double_list<T>& other) : size(0), end_node() {
        head = end_ptr = &end_node;
        *this = other;
    }
    ~double_list() { this->clear(); }

    double_list& operator=(const double_list& other) {
        if (this == &other)
            return *this;
        this->clear();
        for (auto iter = other.begin(); iter != other.end(); iter++) {
            insert_tail(*(iter.ptr->val_ptr));
            this->end_ptr->prev->dual = iter.ptr->dual;
            if (iter.ptr->dual)
                iter.ptr->dual->dual = this->end_ptr->prev;
            iter.ptr->dual = nullptr;
        }
        return *this;
    }

    class iterator {
       public:
        Node* ptr;
        // --------------------------

        iterator(Node* ptr = nullptr) : ptr(ptr) {}
        iterator(const iterator& t) { ptr = t.ptr; }
        ~iterator() {}

        iterator operator++(int) {
            iterator to_return(ptr);
            ptr = ptr->next;
            return to_return;
        }

        iterator& operator++() {
            ptr = ptr->next;
            return *this;
        }

        iterator operator--(int) {
            iterator to_return(ptr);
            ptr = ptr->prev;
            return to_return;
        }

        iterator& operator--() {
            ptr = ptr->prev;
            return *this;
        }
        /**
         * if the iter didn't point to a value
         * throw " invalid"
         */
        T& operator*() const {
            if (!ptr || !ptr->val_ptr) {
                throw std::runtime_error("119:T& operator*()");
            }
            return *(ptr->val_ptr);
        }
        /**
         * other operation
         */
        T* operator->() const noexcept { return ptr->val_ptr; }
        bool operator==(const iterator& rhs) const { return ptr == rhs.ptr; }
        bool operator!=(const iterator& rhs) const { return ptr != rhs.ptr; }
    };
    /**
     * return an iterator to the beginning
     */
    iterator begin() const { return iterator(head); }
    /**
     * return an iterator to the ending
     * in fact, it returns the iterator point to nothing,
     * just after the last element.
     */
    iterator end() const { return iterator(end_ptr); }
    /**
     * if the iter didn't point to anything, do nothing,
     * otherwise, delete the element pointed by the iter
     * and return the iterator point at the same "index"
     * e.g.
     * 	if the origin iterator point at the 2nd element
     * 	the returned iterator also point at the
     *  2nd element of the list after the operation
     *  or nothing if the list after the operation
     *  don't contain 2nd elememt.
     */
    iterator erase(iterator pos) {
        if (pos == iterator() || pos == end())
            return end();
        if (pos == begin()) {
            delete_head();
            return begin();
        }
        Node *to_delete = pos.ptr, *to_return = pos.ptr->next;
        (to_delete->prev)->next = to_delete->next;
        (to_delete->next)->prev = to_delete->prev;
        if (to_delete->dual)
            to_delete->dual->dual = nullptr;
        delete to_delete;
        size--;
        return iterator(to_return);
    }
    void insert_head(const T& val) {
        // modify head
        Node* node_ptr = new Node(new T(val));
        if (head == end_ptr) {
            head = node_ptr;
            node_ptr->next = end_ptr;
        } else {
            node_ptr->next = head;
            head->prev = node_ptr;
            head = node_ptr;
        }
        size++;
    }
    void insert_tail(const T& val) {
        // modify head
        Node* node_ptr = new Node(new T(val));
        if (end_ptr != head) {
            end_ptr->prev->next = node_ptr;
            node_ptr->prev = end_ptr->prev;
            node_ptr->next = end_ptr;
            end_ptr->prev = node_ptr;
        } else {
            head = node_ptr;
            head->next = end_ptr;
            end_ptr->prev = head;
        }
        size++;
    }
    void delete_head() {
        // modify head
        if (head == end_ptr)
            return;
        Node* to_delete = head;
        if (to_delete->dual)
            to_delete->dual->dual = nullptr;
        head = head->next;
        head->prev = nullptr;
        delete to_delete;
        size--;
    }
    void delete_tail() {
        // modify head
        if (head == end_ptr)
            return;
        Node* to_delete = end_ptr->prev;
        if (to_delete->dual)
            to_delete->dual->dual = nullptr;
        if (to_delete->prev)
            to_delete->prev->next = end_ptr;
        else
            head = end_ptr;
        delete to_delete;
        size--;
    }
    void clear() {
        Node* current = head;
        Node* to_delete = current;
        while (current != end_ptr) {
            to_delete = current;
            if (to_delete->dual)
                to_delete->dual->dual = nullptr;
            current = current->next;
            delete to_delete;
        }

        head = end_ptr = &end_node;
    }
    /**
     * if didn't contain anything, return true,
     * otherwise false.
     */
    bool empty() { return !size; }
};

template <class Key,
          class T,
          class Hash = std::hash<Key>,
          class Equal = std::equal_to<Key>>
class hashmap {
   public:
    using value_type = pair<const Key, T>;
    using Node = typename double_list<value_type>::Node;
    using list = double_list<value_type>;
    list* buckets;
    // using the heap space
    Hash hash;
    Equal eq;
    size_t size;
    size_t capacity;
    const float loadFactor = 0.75;

    // --------------------------

    hashmap(size_t _capacity = CAPACITY_DEFAULT)
        : size(0), capacity(_capacity) {
        buckets = new list[_capacity];
    }
    void resize(list*& _buckets, size_t _capacity) {
        // I need to thank Zhangrenhao for reminding me the '&'
        // the delete operation is done in (total_)clear
        _buckets = new list[_capacity];
    }
    hashmap(const hashmap& other) {
        capacity = other.capacity;
        buckets = new list[capacity];
        for (int i = 0; i < capacity; i++)
            buckets[i] = other.buckets[i];
        size = other.size;
    }
    ~hashmap() { this->total_clear(); }

    void clear() {
        for (int i = 0; i < capacity; i++) {
            buckets[i].clear();
        }
        size = 0;
    }
    void total_clear() {
        delete[] buckets;
        size = 0;
    }
    hashmap& operator=(const hashmap& other) {
        if (this == &other)
            return *this;
        this->total_clear();
        size = other.size;
        capacity = other.capacity;
        resize(buckets, capacity);
        for (int i = 0; i < capacity; i++) {
            // maybe to add something
            buckets[i] = other.buckets[i];
        }
        return *this;
    }

    class iterator {
       public:
        Node* ptr;
        // --------------------------

        iterator(Node* ptr = nullptr) : ptr(ptr) {}
        iterator(const iterator& t) { ptr = t.ptr; }
        ~iterator() {}

        /**
         * if point to nothing
         * throw
         */
        value_type& operator*() const {
            if (!ptr || !ptr->val_ptr) {
                throw std::runtime_error("301: value_type& operator*()");
            }
            return *(ptr->val_ptr);
        }

        /**
         * other operation
         */
        value_type* operator->() const noexcept { return ptr->val_ptr; }
        bool operator==(const iterator& rhs) const { return ptr == rhs.ptr; }
        bool operator!=(const iterator& rhs) const { return ptr != rhs.ptr; }
    };

    /**
     * you need to expand the hashmap dynamically
     */
    void expand() {
        hashmap new_map(capacity * 2);
        for (int i = 0; i < capacity; i++) {
            for (auto iter = buckets[i].begin(); iter != buckets[i].end();
                 iter++) {
                auto _iter = new_map.insert(*(iter.ptr->val_ptr)).first;
                _iter.ptr->dual = iter.ptr->dual;
                if (iter.ptr->dual)
                    iter.ptr->dual->dual = _iter.ptr;
                iter.ptr->dual = nullptr;
            }
        }
        *this = new_map;
    }

    /**
     * the iterator point at nothing
     */
    iterator end() const { return iterator(nullptr); }
    /**
     * find, return a pointer point to the value
     * not find, return the end (point to nothing)
     */
    iterator find(const Key& key) const {
        int index = hash(key) % capacity;
        Node* elem = buckets[index].head;
        while (elem != buckets[index].end_ptr) {
            if (elem->val_ptr) {
                if (this->eq(elem->val_ptr->first, key))
                    return iterator(elem);
            }
            elem = elem->next;
        }
        return iterator(nullptr);
    }
    /**
     * already have a value_pair with the same key
     * -> just update the value, return false
     * not find a value_pair with the same key
     * -> insert the value_pair, return true
     */
    sjtu::pair<iterator, bool> insert(const value_type& value_pair) {
        if (size > loadFactor * capacity)
            expand();
        auto iter = find(value_pair.first);
        if (iter == this->end()) {
            int index = hash(value_pair.first) % capacity;
            buckets[index].insert_tail(value_pair);
            size++;
            return {iterator(buckets[index].end_ptr->prev), true};
        }
        (iter.ptr->val_ptr)->second = value_pair.second;
        return {iter, false};
    }
    /**
     * the value_pair exists, remove and return true
     * otherwise, return false
     */
    using Node_iterator = typename sjtu::double_list<value_type>::iterator;
    bool remove(const Key& key) {
        auto iter = find(key);
        int index = hash(key) % capacity;
        if (iter == this->end())
            return false;
        buckets[index].erase(Node_iterator(iter.ptr));
        size--;
        return true;
    }
};

template <class Key,
          class T,
          class Hash = std::hash<Key>,
          class Equal = std::equal_to<Key>>
class linked_hashmap : public hashmap<Key, T, Hash, Equal> {
   public:
    typedef pair<const Key, T> value_type;
    using Node = typename double_list<value_type>::Node;
    double_list<value_type> history;
    //  --------------------------

    linked_hashmap() {}
    linked_hashmap(const linked_hashmap& other) { *this = other; }
    ~linked_hashmap() {}
    linked_hashmap& operator=(const linked_hashmap& other) {
        if (this == &other)
            return *this;
        this->hashmap<Key, T, Hash, Equal>::total_clear();
        history.clear();
        this->capacity = other.capacity;
        this->resize(this->buckets, this->capacity);
        for (auto iter = other.history.begin(); iter != other.history.end();
             iter++) {
            history.insert_tail(*(iter.ptr->val_ptr));
            auto _iter =
                hashmap<Key, T, Hash, Equal>::insert(*(iter.ptr->val_ptr))
                    .first;
            history.end_ptr->prev->dual = _iter.ptr;
            _iter.ptr->dual = history.end_ptr->prev;
        }
        return *this;
    }

    /**
     * return the value connected with the Key(O(1))
     * if the key not found, throw
     */
    T& at(const Key& key) {
        auto iter = find(key);
        if (iter == this->end()) {
            throw std::runtime_error("434:T& at");

            throw std::runtime_error("");
        }
        return iter.ptr->val_ptr->second;
    }
    const T& at(const Key& key) const {
        auto iter = find(key);
        if (iter == this->end()) {
            throw std::runtime_error("443:const T& at");

            throw std::runtime_error("");
        }
        return iter.ptr->val_ptr->second;
    }
    T& operator[](const Key& key) {
        auto iter = find(key);
        if (iter == this->end()) {
            throw std::runtime_error("448 T& operator[]");

            throw std::runtime_error("");
        }
        return iter.ptr->val_ptr->second;
    }
    const T& operator[](const Key& key) const {
        auto iter = find(key);
        if (iter == this->end()) {
            throw std::runtime_error("457:const T& operator[]");

            throw std::runtime_error("");
        }
        return iter.ptr->val_ptr->second;
    }

    class const_iterator;
    class iterator {
       public:
        // the ptr is on history, not on buckets
        Node* ptr;

        // --------------------------
        iterator(Node* ptr = nullptr) : ptr(ptr){};
        iterator(const iterator& other) { ptr = other.ptr; }
        ~iterator() {}

        /**
         * iter++
         */
        iterator operator++(int) {
            iterator to_return(ptr);
            ptr = ptr->next;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return to_return;
        }
        /**
         * ++iter
         */
        iterator& operator++() {
            ptr = ptr->next;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return *this;
        }
        /**
         * iter--
         */
        iterator operator--(int) {
            iterator to_return(ptr);
            ptr = ptr->prev;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return to_return;
        }
        /**
         * --iter
         */
        iterator& operator--() {
            ptr = ptr->prev;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return *this;
        }

        /**
         * if the iter didn't point to a value
         * throw "star invalid"
         */
        value_type& operator*() const {
            if (!ptr || !ptr->val_ptr) {
                throw std::runtime_error("515:value_type& operator*");
            }

            return *(ptr->val_ptr);
        }
        value_type* operator->() const noexcept { return ptr->val_ptr; }

        /**
         * operator to check whether two iterators are same (pointing to the
         * same memory).
         */
        bool operator==(const iterator& rhs) const { return ptr == rhs.ptr; }
        bool operator!=(const iterator& rhs) const { return ptr != rhs.ptr; }
        bool operator==(const const_iterator& rhs) const {
            return ptr == rhs.ptr;
        }
        bool operator!=(const const_iterator& rhs) const {
            return ptr != rhs.ptr;
        }
    };

    class const_iterator {
       public:
        Node* ptr;
        // --------------------------
        const_iterator(Node* ptr = nullptr) : ptr(ptr) {}
        const_iterator(const iterator& other) { ptr = other.ptr; }

        /**
         * iter++
         */
        const_iterator operator++(int) {
            const_iterator to_return(ptr);
            ptr = ptr->next;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return to_return;
        }
        /**
         * ++iter
         */
        const_iterator& operator++() {
            ptr = ptr->next;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return *this;
        }
        /**
         * iter--
         */
        const_iterator operator--(int) {
            const_iterator to_return(ptr);
            ptr = ptr->prev;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return to_return;
        }
        /**
         * --iter
         */
        const_iterator& operator--() {
            ptr = ptr->prev;
            if (!ptr)
                throw std::runtime_error("invalid iterator");
            return *this;
        }

        /**
         * if the iter didn't point to a value
         * throw
         */
        const value_type& operator*() const {
            if (!ptr || !ptr->val_ptr) {
                throw std::runtime_error("583:const value_type& operator*()");
            }

            return *(ptr->val_ptr);
        }
        const value_type* operator->() const noexcept { return ptr->val_ptr; }

        /**
         * operator to check whether two iterators are same (pointing to the
         * same memory).
         */
        bool operator==(const iterator& rhs) const { return ptr == rhs.ptr; }
        bool operator!=(const iterator& rhs) const { return ptr != rhs.ptr; }
        bool operator==(const const_iterator& rhs) const {
            return ptr == rhs.ptr;
        }
        bool operator!=(const const_iterator& rhs) const {
            return ptr != rhs.ptr;
        }
    };

    /**
     * return an iterator point to the first
     * inserted and existed element
     */
    iterator begin() { return iterator(history.head); }

    const_iterator cbegin() const { return const_iterator(history.head); }
    /**
     * return an iterator after the last inserted element
     */
    iterator end() { return iterator(history.end_ptr); }
    const_iterator cend() const { return const_iterator(history.end_ptr); }
    /**
     * if didn't contain anything, return true,
     * otherwise false.
     */
    bool empty() const { return !this->size(); }

    void clear() {
        hashmap<Key, T, Hash, Equal>::clear();
        history.clear();
    }

    size_t size() const { return history.size; }
    /**
     * insert the value_piar
     * if the key of the value_pair exists in the map
     * update the value instead of adding a new element，
     * then the order of the element moved from inner of the
     * list to the head of the list (:rethink)
     * and return false
     * if the key of the value_pair doesn't exist in the map
     * add a new element and return true
     */
    pair<iterator, bool> insert(const value_type& value) {
        // everytime you insert
        // it means "push_back" (not "push_front")
        auto iter = this->hashmap<Key, T, Hash, Equal>::find(value.first);
        // the iter is on the hashmap(buckets)
        if (iter.ptr) {
            // it implies that key already exists.
            iter.ptr->val_ptr->second = value.second;
            history.erase(Node_iterator(iter.ptr->dual));
            history.insert_tail(value);
            iter.ptr->dual = history.end_ptr->prev;
            history.end_ptr->prev->dual = iter.ptr;
            return {iterator(history.end_ptr->prev), false};
        }
        history.insert_tail(value);
        auto to_return = iterator(history.end_ptr->prev);
        auto to_add = (this->hashmap<Key, T, Hash, Equal>::insert(value)).first;
        // problem: when the hashmap expand,
        // the dual isn't maintained
        to_return.ptr->dual = to_add.ptr;
        to_add.ptr->dual = to_return.ptr;
        return {to_return, true};
    }
    /**
     * erase the value_pair pointed by the iterator
     * if the iterator points to nothing
     * throw
     */
    using Node_iterator = typename sjtu::double_list<value_type>::iterator;
    void remove(iterator pos) {
        if (!pos.ptr || !pos.ptr->val_ptr) {
            throw std::runtime_error("676:void remove");
        }
        Node* to_delete = pos.ptr;
        Key key = pos.ptr->val_ptr->first;
        history.erase(Node_iterator(to_delete));
        this->hashmap<Key, T, Hash, Equal>::remove(key);
        return;
    }

    /**
     * return how many value_pairs consist of key
     * this should only return 0 or 1
     */
    size_t count(const Key& key) const {
        auto iter = hashmap<Key, T, Hash, Equal>::find(key);
        if (!iter.ptr)
            return 0;
        return 1;
    }
    /**
     * find the iterator points at the value_pair
     * which consist of key
     * if not find, return the iterator
     * point at nothing
     */
    iterator find(const Key& key) {
        auto iter = this->hashmap<Key, T, Hash, Equal>::find(key);
        if (iter.ptr)
            return iterator(iter.ptr->dual);
        return end();
    }
};

class lru {
    using lmap = sjtu::linked_hashmap<Integer, Matrix<int>, Hash, Equal>;
    using value_type = sjtu::pair<const Integer, Matrix<int>>;
    using Node = typename double_list<value_type>::Node;
    using Node_iterator = typename double_list<value_type>::iterator;
    lmap map;
    size_t max_size;

   public:
    lru(int size) : max_size(size) {
        map.hashmap::size = size;
        map.hashmap::capacity = 4 * size / 3 + 5;
        map.hashmap<Integer, Matrix<int>, Hash, Equal>::total_clear();
        map.history.clear();
        map.hashmap::resize(map.buckets, map.hashmap::capacity);
    }
    ~lru() {}
    /**
     * save the value_pair in the memory
     * delete something in the memory if necessary
     */
    void save(const value_type& v) {
        map.insert(v);
        while (map.size() > max_size)
            map.remove(map.begin());
        return;
    }
    /**
     * return a pointer contain the value
     */
    Matrix<int>* get(const Integer& v) {
        auto iter = map.find(v);
        if (iter == map.end())
            return nullptr;
        auto _iter = map.insert({v, iter.ptr->val_ptr->second}).first;
        return &(_iter.ptr->val_ptr->second);
    }
    /**
     * just print everything in the memory
     * to debug or test.
     * this operation follows the order, but don't
     * change the order.
     */
    void print() {
        sjtu ::linked_hashmap<Integer, Matrix<int>, Hash, Equal>::iterator it;
        for (it = map.begin(); it != map.end(); it++) {
            std ::cout << (*it).first.val << " " << (*it).second << std ::endl;
        }
    }
};
}  // namespace sjtu

#endif