#pragma once

namespace Cesium3DTiles {

    /**
     * A doubly-linked list where the previous and next pointers are embedded directly in
     * the data object.
     * 
     * @tparam T The data object type.
     * @tparam (T::*Next) A member pointer to the field that holds a pointer to the next node.
     * @tparam (T::*Previous) A member pointer to the field that holds a pointer to the previous node.
     */
    template <class T, T* (T::*Next), T* (T::*Previous)>
    class DoublyLinkedList {
    public:
        void remove(T& node) {
            if (node.*Previous) {
                (node.*Previous)->*Next = node.*Next;
                node.*Previous = nullptr;
            } else if (this->_pHead == &node) {
                this->_pHead = node.*Next;
            }

            if (node.*Next) {
                (node.*Next)->*Previous = node.*Previous;
                node.*Next = nullptr;
            } else if (this->_pTail == &node) {
                this->_pTail = node.*Previous;
            }
        }

        void insertAfter(T& after, T& node) {
            this->remove(node);
            
            node.*Previous = &after;
            node.*Next = after.*Next;
            after.*Next = &node;

            if (node.*Next) {
                (node.*Next)->*Previous = &node;
            }
            
            if (this->_pTail == &after) {
                this->_pTail = &node;
            }
        }

        void insertBefore(T& before, T& node) {
            this->remove(node);

            node.*Previous = before.*Previous;
            node.*Next = &before;
            before.*Previous = &node;

            if (node.*Previous) {
                (node.*Previous)->*Next = &node;
            }

            if (this->_pHead == &before) {
                this->_pHead = &node;
            }
        }

        void insertAtHead(T& node) {
            this->remove(node);

            if (this->_pHead) {
                this->_pHead->*Previous = &node;
            } else {
                this->_pTail = &node;
            }
            this->_pHead = &node;
        }

        void insertAtTail(T& node) {
            this->remove(node);

            if (this->_pTail) {
                this->_pTail->*Next = &node;
            } else {
                this->_pHead = &node;
            }
            this->_pTail = &node;
        }

        T* next(T& node) {
            return node.*Next;
        }

        T* previous(T& node) {
            return node.*Previous;
        }

    private:
        T* _pHead;
        T* _pTail;
    };

}
