/**
 * \file
 * \brief
 * \author
 */
#include "list.h"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness_c.h>

#include <algorithm>
#include <array>

TEST_GROUP(ListTests){};

TEST(ListTests, EmptyList)
{
    LIST_HEAD(head);

    CHECK_TRUE(list_head_empty(&head));
}

TEST(ListTests, AddTwoItem)
{
    LIST_HEAD(head);

    list_node node1;
    list_node node2;

    LIST_NODE_INIT(&node1);
    LIST_NODE_INIT(&node2);
    list_head_add(&head, &node1);
    list_head_add(&head, &node2);

    CHECK_FALSE(list_head_empty(&head));
    const list_node *const checks[] = {
        &node2,
        &node1,
    };

    list_node *it;
    size_t id = 0;

    LIST_FOR_EACH(it, &head)
    {
        CHECK_EQUAL_C_POINTER(checks[id++], it);
    }

    CHECK(id == 2);
}

TEST(ListTests, AddTwoItemsAndRemoveOne)
{
    LIST_HEAD(head);

    list_node node1;
    list_node node2;

    LIST_NODE_INIT(&node1);
    LIST_NODE_INIT(&node2);
    list_head_add(&head, &node1);
    list_head_add(&head, &node2);

    CHECK_FALSE(list_head_empty(&head));
    const list_node *const checks[] = {
        &node2,
        &node1,
    };

    list_node *it;
    size_t id = 0;

    LIST_FOR_EACH(it, &head)
    {
        CHECK_EQUAL_C_POINTER(checks[id++], it);
    }

    CHECK(id == 2);

    list_head_del(&node2);
    id = 0;

    const list_node *const checks2[] = {
        &node1,
    };

    LIST_FOR_EACH(it, &head)
    {
        CHECK_EQUAL_C_POINTER(checks2[id++], it);
    }

    CHECK(id == 1);
    CHECK_FALSE(list_head_empty(&head));
}

TEST(ListTests, AddMultipleItemsAndRemoveAll)
{
    LIST_HEAD(head);
    std::array<list_node, 10> nodes{};

    std::for_each(std::begin(nodes), std::end(nodes), [&head](list_node &node) {
        LIST_NODE_INIT(&node);
        list_head_add(&head, &node);
    });

    CHECK_FALSE(list_head_empty(&head));
    list_node *it;
    size_t id = nodes.size() - 1;

    LIST_FOR_EACH(it, &head)
    {
        CHECK_EQUAL_C_POINTER(&nodes[id], it);

        if (id != 0) {
            --id;
        }
    }

    CHECK(id == 0);
    std::for_each(std::begin(nodes), std::end(nodes), [](list_node &node) {
        list_head_del(&node);
    });
    CHECK_TRUE(list_head_empty(&head));
}

int main(int argc, char *argv[])
{
    return RUN_ALL_TESTS(argc, argv);
}
