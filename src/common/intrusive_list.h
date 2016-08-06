/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <iterator>
#include <memory>

#include "common/assert.h"
#include "common/common_types.h"

namespace Dynarmic {
namespace Common {

template <typename T> class IntrusiveListNode;
template <typename T> class IntrusiveList;
template <typename T> class IntrusiveListIterator;
template <typename T> class IntrusiveListConstIterator;

template <typename T>
class IntrusiveListNode {
public:
    IntrusiveListNode() : next(this), prev(this) {}
    void UnlinkFromList() {
        prev->next = next;
        next->prev = prev;
#if !defined(NDEBUG)
        next = prev = nullptr;
#endif
    }
private:
    friend class IntrusiveList<T>;
    friend class IntrusiveListIterator<T>;
    friend class IntrusiveListConstIterator<T>;
    IntrusiveListNode<T>* next;
    IntrusiveListNode<T>* prev;
};

template <typename T>
class IntrusiveList {
public:
    /**
     * Add an entry to the start of the list.
     * @param node Node to add to the list.
     */
    void Prepend(T& node) {
        AddAfter(root.get(), static_cast<IntrusiveListNode<T>*>(&node));
    }

    /**
     * Add an entry to the end of the list
     * @param node  Node to add to the list.
     */
    void Append(T& node) {
        AddBefore(root.get(), static_cast<IntrusiveListNode<T>*>(&node));
    }

    /**
     * Add an entry after an existing node in this list
     * @param existing_node Node to add new_node after. Must already be member of the list.
     * @param new_node Node to add to the list.
     */
    void AddAfter(T& existing, T& node) {
        AddAfter(static_cast<IntrusiveListNode<T>*>(&existing), static_cast<IntrusiveListNode<T>*>(&node));
    }

    /**
     * Add an entry before an existing node in this list
     * @param existing_node Node to add new_node before. Must already be member of the list.
     * @param new_node Node to add to the list.
     */
    void AddBefore(T& existing, T& node) {
        AddBefore(static_cast<IntrusiveListNode<T>*>(&existing), static_cast<IntrusiveListNode<T>*>(&node));
    }

    /**
     * Removes node from list
     * @param node Node to remove from list.
     */
    void Remove(T& node) {
        node.UnlinkFromList();
    }

    /**
     * Is this list empty?
     * @returns true if there are no nodes in this list.
     */
    bool IsEmpty() {
        return root->next == root.get();
    }

    IntrusiveListIterator<T> begin();
    IntrusiveListIterator<T> end();
    IntrusiveListIterator<T> erase(const IntrusiveListIterator<T>&);
    IntrusiveListIterator<T> iterator_to(T&);

    IntrusiveListConstIterator<T> begin() const;
    IntrusiveListConstIterator<T> end() const;
    IntrusiveListConstIterator<T> iterator_to(T&) const;

private:
    friend class IntrusiveListIterator<T>;
    friend class IntrusiveListConstIterator<T>;

    void AddAfter(IntrusiveListNode<T>* existing_node, IntrusiveListNode<T>* new_node) {
        new_node->next = existing_node->next;
        new_node->prev = existing_node;
        existing_node->next->prev = new_node;
        existing_node->next = new_node;
    }

    void AddBefore(IntrusiveListNode<T>* existing_node, IntrusiveListNode<T>* new_node) {
        new_node->next = existing_node;
        new_node->prev = existing_node->prev;
        existing_node->prev->next = new_node;
        existing_node->prev = new_node;
    }

    std::shared_ptr<IntrusiveListNode<T>> root = std::make_shared<IntrusiveListNode<T>>();
};

template <typename T>
class IntrusiveListIterator {
public:
    IntrusiveListIterator() : root(nullptr), node(nullptr) {}
    IntrusiveListIterator(IntrusiveList<T>* list, IntrusiveListNode<T>* node) : root(list->root.get()), node(node) {}
    IntrusiveListIterator(const IntrusiveListIterator& other) : root(other.root), node(other.node) {}
    IntrusiveListIterator& operator=(IntrusiveListIterator other) {
        std::swap(root, other.root);
        std::swap(node, other.node);
        return *this;
    }

    IntrusiveListIterator& operator++() {
        node = node == root ? node : node->next;
        return *this;
    }
    IntrusiveListIterator operator++(int) {
        IntrusiveListIterator it(*this);
        node = node == root ? node : node->next;
        return it;
    }
    IntrusiveListIterator& operator--() {
        node = node->prev == root ? node : node->prev;
        return *this;
    }
    IntrusiveListIterator operator--(int) {
        IntrusiveListIterator it(*this);
        node = node->prev == root ? node : node->prev;
        return it;
    }

    bool operator==(const IntrusiveListIterator& other) const {
        DEBUG_ASSERT(root == other.root);
        return node == other.node;
    }
    bool operator!=(const IntrusiveListIterator& other) const {
        return !(*this == other);
    }

    const T& operator*() const {
        DEBUG_ASSERT(node != root);
        return *reinterpret_cast<T*>(node);
    }
    T& operator*() {
        DEBUG_ASSERT(node != root);
        return *reinterpret_cast<T*>(node);
    }
    T* operator->() {
        DEBUG_ASSERT(node != root);
        return reinterpret_cast<T*>(node);
    }
    const T* operator->() const {
        DEBUG_ASSERT(node != root);
        return reinterpret_cast<T*>(node);
    }

private:
    friend class IntrusiveList<T>;
    IntrusiveListNode<T>* root;
    IntrusiveListNode<T>* node;
};

template <typename T>
class IntrusiveListConstIterator {
public:
    IntrusiveListConstIterator() : root(nullptr), node(nullptr) {}
    IntrusiveListConstIterator(const IntrusiveList<T>* list, IntrusiveListNode<T>* node) : root(list->root.get()), node(node) {}
    IntrusiveListConstIterator(const IntrusiveListConstIterator& other) : root(other.root), node(other.node) {}
    IntrusiveListConstIterator& operator=(IntrusiveListConstIterator other) {
        std::swap(root, other.root);
        std::swap(node, other.node);
        return *this;
    }

    IntrusiveListConstIterator& operator++() {
        node = node == root ? node : node->next;
        return *this;
    }
    IntrusiveListConstIterator operator++(int) {
        IntrusiveListConstIterator it(*this);
        node = node == root ? node : node->next;
        return it;
    }
    IntrusiveListConstIterator& operator--() {
        node = node->prev == root ? node : node->prev;
        return *this;
    }
    IntrusiveListConstIterator operator--(int) {
        IntrusiveListConstIterator it(*this);
        node = node->prev == root ? node : node->prev;
        return it;
    }

    bool operator==(const IntrusiveListConstIterator& other) const {
        DEBUG_ASSERT(root == other.root);
        return node == other.node;
    }
    bool operator!=(const IntrusiveListConstIterator& other) const {
        return !(*this == other);
    }

    const T& operator*() const {
        DEBUG_ASSERT(node != root);
        return *reinterpret_cast<T*>(node);
    }
    const T* operator->() const {
        DEBUG_ASSERT(node != root);
        return reinterpret_cast<T*>(node);
    }

private:
    friend class IntrusiveList<T>;
    IntrusiveListNode<T>* root;
    IntrusiveListNode<T>* node;
};

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::begin() {
    return IntrusiveListIterator<T>(this, root->next);
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::end() {
    return IntrusiveListIterator<T>(this, root.get());
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::erase(const IntrusiveListIterator<T>& it) {
    DEBUG_ASSERT(it.root == root.get() && it.node != it.root);
    IntrusiveListNode<T>* to_remove = it.node;
    IntrusiveListIterator<T> ret = it;
    ++ret;
    to_remove->UnlinkFromList();
    return ret;
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::iterator_to(T& item) {
    return IntrusiveListIterator<T>(this, static_cast<IntrusiveListNode<T>*>(&item));
}

template <typename T>
IntrusiveListConstIterator<T> IntrusiveList<T>::begin() const {
    return IntrusiveListConstIterator<T>(this, root->next);
}

template <typename T>
IntrusiveListConstIterator<T> IntrusiveList<T>::end() const {
    return IntrusiveListConstIterator<T>(this, root.get());
}

template <typename T>
IntrusiveListConstIterator<T> IntrusiveList<T>::iterator_to(T& item) const {
    return IntrusiveListConstIterator<T>(this, static_cast<IntrusiveListNode<T>*>(&item));
}

} // namespace Common
} // namespace Dynarmic
