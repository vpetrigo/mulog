/**
 * \file
 * \brief Linked list tests
 * \author Vladimir Petrigo
 */
#include "list.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>

TEST_CASE("ListTests - EmptyList", "[list]")
{
    constexpr LIST_HEAD(head);

    REQUIRE(list_head_empty(&head));
}

TEST_CASE("ListTests - AddOneItem", "[list]")
{
    LIST_HEAD(head);

    list_node node{};

    LIST_NODE_INIT(&node);
    list_head_add(&head, &node);
    REQUIRE_FALSE(list_head_empty(&head));
    list_head_del(&node);
    REQUIRE(list_head_empty(&head));
}

TEST_CASE("ListTests - AddTwoItem", "[list]")
{
    LIST_HEAD(head);

    list_node node1{};
    list_node node2{};

    LIST_NODE_INIT(&node1);
    LIST_NODE_INIT(&node2);
    list_head_add(&head, &node1);
    list_head_add(&head, &node2);

    REQUIRE_FALSE(list_head_empty(&head));
    const list_node *const checks[] = {
        &node2,
        &node1,
    };

    list_node *it;
    size_t id = 0;

    LIST_FOR_EACH(it, &head)
    {
        REQUIRE(checks[id++] == it);
    }

    REQUIRE(id == 2);
}

TEST_CASE("ListTests - AddTwoItemsAndRemoveOne", "[list]")
{
    LIST_HEAD(head);

    list_node node1{};
    list_node node2{};

    LIST_NODE_INIT(&node1);
    LIST_NODE_INIT(&node2);
    list_head_add(&head, &node1);
    list_head_add(&head, &node2);

    REQUIRE_FALSE(list_head_empty(&head));
    const list_node *const checks[] = {
        &node2,
        &node1,
    };

    list_node *it;
    size_t id = 0;

    LIST_FOR_EACH(it, &head)
    {
        REQUIRE(checks[id++] == it);
    }

    REQUIRE(id == 2);

    list_head_del(&node2);
    id = 0;

    const list_node *const checks2[] = {
        &node1,
    };

    LIST_FOR_EACH(it, &head)
    {
        REQUIRE(checks2[id++] == it);
    }

    REQUIRE(id == 1);
    REQUIRE_FALSE(list_head_empty(&head));
}

TEST_CASE("ListTests - AddMultipleItemsAndRemoveAll", "[list]")
{
    LIST_HEAD(head);
    std::array<list_node, 10> nodes{};

    std::for_each(std::begin(nodes), std::end(nodes), [&head](list_node &node) {
        LIST_NODE_INIT(&node);
        list_head_add(&head, &node);
    });

    REQUIRE_FALSE(list_head_empty(&head));
    list_node *it;
    size_t id = nodes.size() - 1;

    LIST_FOR_EACH(it, &head)
    {
        REQUIRE(&nodes[id] == it);

        if (id != 0) {
            --id;
        }
    }

    REQUIRE(id == 0);
    std::for_each(std::begin(nodes), std::end(nodes), [](list_node &node) {
        list_head_del(&node);
    });
    REQUIRE(list_head_empty(&head));
}
