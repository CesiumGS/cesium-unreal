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

        DoublyLinkedListPointers& operator=(const DoublyLinkedListPointers& rhs) {
            return *this;
        }

    private:
        template <class T, DoublyLinkedListPointers<T> (T::*Pointers)>
        friend class DoublyLinkedList;

        T* pNext;
        T* pPrevious;
    };

    /**
     * A doubly-linked list where the previous and next pointers are embedded directly in
     * the data object.
     * 
     * @tparam T The data object type.
     * @tparam (T::*Next) A member pointer to the field that holds a pointer to the next node.
     * @tparam (T::*Previous) A member pointer to the field that holds a pointer to the previous node.
     */
    template <class T, DoublyLinkedListPointers<T> (T::*Pointers)>
    class DoublyLinkedList {
    public:
        void remove(T& node) {
            DoublyLinkedListPointers<T>& nodePointers = node.*Pointers;

            if (nodePointers.pPrevious) {
                DoublyLinkedListPointers<T>& previousPointers = nodePointers.pPrevious->*Pointers;
                previousPointers.pNext = nodePointers.pNext;
                nodePointers.pPrevious = nullptr;
            } else if (this->_pHead == &node) {
                this->_pHead = nodePointers.pNext;
            }

            if (nodePointers.pNext) {
                DoublyLinkedListPointers<T>& nextPointers = nodePointers.pNext->*Pointers;
                nextPointers.pPrevious = nodePointers.pPrevious;
                nodePointers.pNext = nullptr;
            } else if (this->_pTail == &node) {
                this->_pTail = nodePointers.pPrevious;
            }
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
        }

        void insertAtHead(T& node) {
            this->remove(node);

            if (this->_pHead) {
                (this->_pHead->*Pointers).pPrevious = &node;
            } else {
                this->_pTail = &node;
            }
            this->_pHead = &node;
        }

        void insertAtTail(T& node) {
            this->remove(node);

            if (this->_pTail) {
                (this->_pTail->*Pointers).pNext = &node;
            } else {
                this->_pHead = &node;
            }
            this->_pTail = &node;
        }

        T* next(T& node) {
            return (node.*Pointers).pNext;
        }

        T* previous(T& node) {
            return (node.*Pointers).pPrevious;
        }

    private:
        T* _pHead;
        T* _pTail;
    };

}
