#include "catch2/catch.hpp"
#include "CesiumUtility/DoublyLinkedList.h"

using namespace CesiumUtility;

namespace {

    class TestNode {
    public:
        TestNode(uint32_t valueParam) :
            value(valueParam),
            links()
        {}

        uint32_t value;
        DoublyLinkedListPointers<TestNode> links;
    };


    void assertOrder(DoublyLinkedList<TestNode, &TestNode::links>& linkedList, const std::vector<uint32_t>& expectedOrder) {
        CHECK(linkedList.size() == expectedOrder.size());
        if (expectedOrder.size() > 0) {
            REQUIRE(linkedList.head() != nullptr);
            REQUIRE(linkedList.tail() != nullptr);

            CHECK(linkedList.head()->value == expectedOrder.front());
            CHECK(linkedList.tail()->value == expectedOrder.back());
            
            TestNode* pCurrent = linkedList.head();

            for (size_t i = 0; i < expectedOrder.size(); ++i) {
                REQUIRE(pCurrent != nullptr);
                CHECK(pCurrent->value == expectedOrder[i]);

                if (i == 0) {
                    CHECK(linkedList.previous(*pCurrent) == nullptr);
                } else {
                    CHECK(linkedList.previous(*pCurrent) != nullptr);
                    CHECK(linkedList.previous(*pCurrent)->value == expectedOrder[i - 1]);
                }

                if (i == expectedOrder.size() - 1) {
                    CHECK(linkedList.next(*pCurrent) == nullptr);
                } else {
                    CHECK(linkedList.next(*pCurrent) != nullptr);
                    CHECK(linkedList.next(*pCurrent)->value == expectedOrder[i + 1]);
                }

                pCurrent = linkedList.next(*pCurrent);
            }

            CHECK(linkedList.next(nullptr) != nullptr);
            CHECK(linkedList.next(nullptr)->value == expectedOrder.front());

            CHECK(linkedList.previous(nullptr) != nullptr);
            CHECK(linkedList.previous(nullptr)->value == expectedOrder.back());
        } else {
            CHECK(linkedList.head() == nullptr);
            CHECK(linkedList.tail() == nullptr);
            CHECK(linkedList.next(nullptr) == nullptr);
            CHECK(linkedList.previous(nullptr) == nullptr);
        }
    }

}

TEST_CASE("DoublyLinkedList") {
    DoublyLinkedList<TestNode, &TestNode::links> linkedList;

    TestNode one(1);
    TestNode two(2);
    TestNode three(3);
    TestNode four(4);

    linkedList.insertAtTail(one);
    linkedList.insertAtTail(two);
    linkedList.insertAtTail(three);
    linkedList.insertAtTail(four);

    assertOrder(linkedList, { 1, 2, 3, 4 });

    SECTION("insertAtHead") {
        TestNode newNode(5);
        linkedList.insertAtHead(newNode);
        assertOrder(linkedList, { 5, 1, 2, 3, 4 });
    }

    SECTION("insertBefore at head") {
        TestNode newNode(5);
        linkedList.insertBefore(one, newNode);
        assertOrder(linkedList, { 5, 1, 2, 3, 4 });
    }

    SECTION("insertAfter at head") {
        TestNode newNode(5);
        linkedList.insertAfter(one, newNode);
        assertOrder(linkedList, { 1, 5, 2, 3, 4 });
    }

    SECTION("insertAtTail") {
        TestNode newNode(5);
        linkedList.insertAtTail(newNode);
        assertOrder(linkedList, { 1, 2, 3, 4, 5 });
    }

    SECTION("insertAfter at tail") {
        TestNode newNode(5);
        linkedList.insertAfter(four, newNode);
        assertOrder(linkedList, { 1, 2, 3, 4, 5 });
    }

    SECTION("insertBefore at tail") {
        TestNode newNode(5);
        linkedList.insertBefore(four, newNode);
        assertOrder(linkedList, { 1, 2, 3, 5, 4 });
    }

    SECTION("insertBefore in middle") {
        TestNode newNode(5);
        linkedList.insertBefore(three, newNode);
        assertOrder(linkedList, { 1, 2, 5, 3, 4 });
    }

    SECTION("insertAfter in middle") {
        TestNode newNode(5);
        linkedList.insertAfter(three, newNode);
        assertOrder(linkedList, { 1, 2, 3, 5, 4 });
    }

    SECTION("insertAtTail when already there") {
        linkedList.insertAtTail(four);
        assertOrder(linkedList, { 1, 2, 3, 4 });
    }

    SECTION("insertAtHead when already there") {
        linkedList.insertAtHead(one);
        assertOrder(linkedList, { 1, 2, 3, 4 });
    }

    SECTION("insertBefore when already there") {
        linkedList.insertBefore(two, one);
        assertOrder(linkedList, { 1, 2, 3, 4 });

        linkedList.insertBefore(three, two);
        assertOrder(linkedList, { 1, 2, 3, 4 });
    
        linkedList.insertBefore(four, three);
        assertOrder(linkedList, { 1, 2, 3, 4 });
    }

    SECTION("insertAfter when already there") {
        linkedList.insertAfter(one, two);
        assertOrder(linkedList, { 1, 2, 3, 4 });

        linkedList.insertAfter(two, three);
        assertOrder(linkedList, { 1, 2, 3, 4 });
    
        linkedList.insertAfter(three, four);
        assertOrder(linkedList, { 1, 2, 3, 4 });
    }
}
