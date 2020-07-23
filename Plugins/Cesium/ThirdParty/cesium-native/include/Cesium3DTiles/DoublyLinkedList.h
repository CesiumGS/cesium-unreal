#pragma once

namespace Cesium3DTiles {

    template <class T>
    class DoublyLinkedListPointers {
    public:
        DoublyLinkedListPointers() :
            pNext(nullptr),
            pPrevious(nullptr)
        {}

        // Following the example of boost::instrusive::list's list_member_hook, the
        // copy constructor and assignment operator do nothing.
        // https://www.boost.org/doc/libs/1_73_0/doc/html/boost/intrusive/list_member_hook.html
        DoublyLinkedListPointers(DoublyLinkedListPointers& rhs) :
            DoublyLinkedListPointers()
        {}

        DoublyLinkedListPointers& operator=(const DoublyLinkedListPointers& /*rhs*/) {
            return *this;
        }

    private:
        template <class TElement, DoublyLinkedListPointers<TElement> (TElement::*Pointers)>
        friend class DoublyLinkedList;

        T* pNext;
        T* pPrevious;
    };

    /**
     * A doubly-linked list where the previous and next pointers are embedded directly in
     * the data object.
     * 
     * @tparam T The data object type.
     * @tparam (T::*Pointers) A member pointer to the field that holds the links to the previous and next nodes.
     */
    template <class T, DoublyLinkedListPointers<T> (T::*Pointers)>
    class DoublyLinkedList {
    public:
        void remove(T& node) {
            DoublyLinkedListPointers<T>& nodePointers = node.*Pointers;

            if (nodePointers.pPrevious) {
                DoublyLinkedListPointers<T>& previousPointers = nodePointers.pPrevious->*Pointers;
                previousPointers.pNext = nodePointers.pNext;
                --this->_size;
            } else if (this->_pHead == &node) {
                this->_pHead = nodePointers.pNext;
                --this->_size;
            }

            if (nodePointers.pNext) {
                DoublyLinkedListPointers<T>& nextPointers = nodePointers.pNext->*Pointers;
                nextPointers.pPrevious = nodePointers.pPrevious;
            } else if (this->_pTail == &node) {
                this->_pTail = nodePointers.pPrevious;
            }

            nodePointers.pPrevious = nullptr;
            nodePointers.pNext = nullptr;
        }

        void insertAfter(T& after, T& node) {
            this->remove(node);
            
            DoublyLinkedListPointers<T>& afterPointers = after.*Pointers;
            DoublyLinkedListPointers<T>& nodePointers = node.*Pointers;

            nodePointers.pPrevious = &after;
            nodePointers.pNext = afterPointers.pNext;
            afterPointers.pNext = &node;

            if (nodePointers.pNext) {
                DoublyLinkedListPointers<T>& nextPointers = nodePointers.pNext->*Pointers;
                nextPointers.pPrevious = &node;
            }
            
            if (this->_pTail == &after) {
                this->_pTail = &node;
            }

            ++this->_size;
        }

        void insertBefore(T& before, T& node) {
            this->remove(node);

            DoublyLinkedListPointers<T>& beforePointers = before.*Pointers;
            DoublyLinkedListPointers<T>& nodePointers = node.*Pointers;

            nodePointers.pPrevious = beforePointers.pPrevious;
            nodePointers.pNext = &before;
            beforePointers.pPrevious = &node;

            if (nodePointers.pPrevious) {
                DoublyLinkedListPointers<T>& previousPointers = nodePointers.pPrevious->*Pointers;
                previousPointers.pNext = &node;
            }

            if (this->_pHead == &before) {
                this->_pHead = &node;
            }

            ++this->_size;
        }

        void insertAtHead(T& node) {
            this->remove(node);

            if (this->_pHead) {
                (this->_pHead->*Pointers).pPrevious = &node;
                (node.*Pointers).pNext = this->_pHead;
            } else {
                this->_pTail = &node;
            }
            this->_pHead = &node;

            ++this->_size;
        }

        void insertAtTail(T& node) {
            this->remove(node);

            if (this->_pTail) {
                (this->_pTail->*Pointers).pNext = &node;
                (node.*Pointers).pPrevious = this->_pTail;
            } else {
                this->_pHead = &node;
            }
            this->_pTail = &node;

            ++this->_size;
        }

        size_t size() {
            return this->_size;
        }

        T* head() {
            return this->_pHead;
        }

        T* tail() {
            return this->_pTail;
        }

        T* next(T& node) {
            return (node.*Pointers).pNext;
        }

        T* next(T* pNode) {
            return pNode ? this->next(*pNode) : this->_pHead;
        }

        T* previous(T& node) {
            return (node.*Pointers).pPrevious;
        }

        T* previous(T* pNode) {
            return pNode ? this->previous(*pNode) : this->_pTail;
        }

    private:
        size_t _size = 0;
        T* _pHead = nullptr;
        T* _pTail = nullptr;
    };

}
