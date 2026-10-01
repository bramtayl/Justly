#pragma once

#include <ranges>

#include "other/helpers.hpp"

// visits only element children, skipping text and comment nodes
struct XMLChildIterator {
  using difference_type = std::ptrdiff_t;
  using value_type = xmlNode;

  xmlNode* node_pointer = nullptr;

  [[nodiscard]] auto operator*() const -> xmlNode& {
    return get_reference(node_pointer);
  }

  auto operator++() -> XMLChildIterator& {
    node_pointer = xmlNextElementSibling(node_pointer);
    return *this;
  }

  auto operator++(int) -> XMLChildIterator {
    auto old_iterator = *this;
    ++*this;
    return old_iterator;
  }

  [[nodiscard]] auto operator==(const XMLChildIterator& other) const
      -> bool = default;
};

struct XMLChildren : std::ranges::view_interface<XMLChildren> {
  xmlNode* first_pointer = nullptr;

  [[nodiscard]] auto begin() const -> XMLChildIterator {
    return {.node_pointer = first_pointer};
  }

  [[nodiscard]] static auto end() -> XMLChildIterator { return {}; }
};

[[nodiscard]] inline auto get_xml_children(xmlNode& node) -> XMLChildren {
  XMLChildren children;
  children.first_pointer = xmlFirstElementChild(&node);
  return children;
}
