/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <iterator>
#include <memory>
#include <type_traits>

#include "common/assert.h"
#include "common/common_types.h"

namespace Dynarmic {
namespace Common {

template <typename T> class IntrusiveList;
template <typename T> class IntrusiveListIterator;

template <typename T>
class IntrusiveListNode {
public:
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
    friend class IntrusiveListIterator<const T>;

    IntrusiveListNode* next = this;
    IntrusiveListNode* prev = this;
};

template <typename T>
class IntrusiveList {
public:
    /**
     * Add an entry to the start of the list.
     * @param node Node to add to the list.
     */
    void Prepend(T& node) {
        AddAfter(root.get(), &node);
    }

    /**
     * Add an entry to the end of the list
     * @param node  Node to add to the list.
     */
    void Append(T& node) {
        AddBefore(root.get(), &node);
    }

    /**
     * Add an entry after an existing node in this list
     * @param existing_node Node to add new_node after. Must already be member of the list.
     * @param new_node Node to add to the list.
     */
    void AddAfter(T& existing, T& node) {
        AddAfter(&existing, &node);
    }

    /**
     * Add an entry before an existing node in this list
     * @param existing_node Node to add new_node before. Must already be member of the list.
     * @param new_node Node to add to the list.
     */
    void AddBefore(T& existing, T& node) {
        AddBefore(&existing, &node);
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

    IntrusiveListIterator<T> begin() const;
    IntrusiveListIterator<T> end() const;
    IntrusiveListIterator<T> iterator_to(T&) const;

private:
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
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = value_type*;
    using const_pointer     = const value_type*;
    using reference         = value_type&;
    using const_reference   = const value_type&;

    // If value_type is const, we want "const IntrusiveListNode<value_type>", not "const IntrusiveListNode<const value_type>"
    using node_type         = std::conditional_t<std::is_const<value_type>::value,
                                                 const IntrusiveListNode<std::remove_const_t<value_type>>,
                                                 IntrusiveListNode<value_type>>;
    using node_pointer      = node_type*;
    using node_reference    = node_type&;

    IntrusiveListIterator() = default;
    IntrusiveListIterator(const IntrusiveListIterator& other) = default;
    IntrusiveListIterator& operator=(const IntrusiveListIterator& other) = default;

    IntrusiveListIterator(node_pointer list_root, node_pointer node) : root(list_root), node(node) {
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

    reference operator*() const {
        DEBUG_ASSERT(node != root);
        return static_cast<T&>(*node);
    }
    pointer operator->() const {
        DEBUG_ASSERT(node != root);
        return std::addressof(operator*());
    }

private:
    friend class IntrusiveList<T>;
    node_pointer root = nullptr;
    node_pointer node = nullptr;
};

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::begin() {
    return IntrusiveListIterator<T>(root.get(), root->next);
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::end() {
    return IntrusiveListIterator<T>(root.get(), root.get());
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
    return IntrusiveListIterator<T>(root.get(), &item);
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::begin() const {
    return IntrusiveListIterator<T>(root.get(), root->next);
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::end() const {
    return IntrusiveListIterator<T>(root.get(), root.get());
}

template <typename T>
IntrusiveListIterator<T> IntrusiveList<T>::iterator_to(T& item) const {
    return IntrusiveListIterator<T>(root.get(), &item);
}

} // namespace Common
} // namespace Dynarmic
