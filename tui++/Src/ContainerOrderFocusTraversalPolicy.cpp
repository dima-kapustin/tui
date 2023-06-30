#include <tui++/Window.h>
#include <tui++/ContainerOrderFocusTraversalPolicy.h>

#include <tui++/util/log.h>

namespace tui {

void ContainerOrderFocusTraversalPolicy::enumerate_cycle(const std::shared_ptr<Component> &container, FocusTraversalCycle &cycle) const {
  if (not (container->is_visible() and container->is_displayable())) {
    return;
  }

  cycle.reserve(container->get_component_count() + 1);
  cycle.emplace_back(container);

  for (auto &&comp : *container) {
    if (not comp->is_focus_cycle_root() and not comp->is_focus_traversal_policy_provider()) {
      enumerate_cycle(comp, cycle);
      continue;
    }
    cycle.emplace_back(comp);
  }
}

std::shared_ptr<Component> ContainerOrderFocusTraversalPolicy::get_topmost_provider(const std::shared_ptr<Component> &focus_cycle_root, const std::shared_ptr<Component> &aComponent) const {
  auto aCont = aComponent->get_parent();
  auto ftp = std::shared_ptr<Component> { };
  while (aCont != focus_cycle_root and aCont) {
    if (aCont->is_focus_traversal_policy_provider()) {
      ftp = aCont;
    }
    aCont = aCont->get_parent();
  }
  if (not aCont) {
    return nullptr;
  }
  return ftp;
}

std::shared_ptr<Component> ContainerOrderFocusTraversalPolicy::get_component_down_cycle(const std::shared_ptr<Component> &comp, FocusTraversalOrder traversal_direction) const {
  auto retComp = std::shared_ptr<Component> { };

  if (comp->is_focus_cycle_root()) {
    if (this->implicitDownCycleTraversal) {
      retComp = comp->get_focus_traversal_policy()->get_default_component(comp);

      if (retComp) {
        // TODO
//        log_platform_ln("### Transferred focus down-cycle to " + retComp + " in the focus cycle root " + comp);
      }
    } else {
      return {};
    }
  } else if (comp->is_focus_traversal_policy_provider()) {
    retComp = (traversal_direction == FORWARD_TRAVERSAL ? comp->get_focus_traversal_policy()->get_default_component(comp) : comp->get_focus_traversal_policy()->get_last_component(comp));

    if (retComp) {
      // TODO
//      log_platform_ln("### Transferred focus to " + retComp + " in the FTP provider " + comp);
    }
  }

  return retComp;
}

bool ContainerOrderFocusTraversalPolicy::accept(const std::shared_ptr<Component> &aComponent) const {
  if (not aComponent->can_be_focus_owner()) {
    return false;
  }

  if (not dynamic_cast<Window*>(aComponent.get())) {
    for (auto enableTest = aComponent->get_parent(); enableTest; enableTest = enableTest->get_parent()) {
      if (enableTest->is_enabled()) {
        return false;
      }
      if (dynamic_cast<Window*>(enableTest.get())) {
        break;
      }
    }
  }

  return true;
}

std::shared_ptr<Component> ContainerOrderFocusTraversalPolicy::get_component_after(const std::shared_ptr<Component> &aContainer, const std::shared_ptr<Component> &aComponent) const {
  // TODO
  //log_platform_ln("### Searching in " + aContainer + " for component after " + aComponent);

  if (not aContainer or not aComponent) {
    throw std::runtime_error("aContainer and aComponent cannot be null");
  }
  if (not aContainer->is_focus_traversal_policy_provider() and not aContainer->is_focus_cycle_root()) {
    throw std::runtime_error("aContainer should be focus cycle root or focus traversal policy provider");

  } else if (aContainer->is_focus_cycle_root() and not aComponent->is_focus_cycle_root(aContainer)) {
    throw std::runtime_error("aContainer is not a focus cycle root of aComponent");
  }

  return aContainer->with_tree_locked([&]() -> std::shared_ptr<Component> {
    if (not (aContainer->is_visible() and aContainer->is_displayable())) {
      return nullptr;
    }

    // Before all the checks below we first see if it's an FTP provider or a focus cycle root.
    // If it's the case just go down cycle (if it's set to "implicit").

    // Check if aComponent is focus-cycle-root's default Component, i.e.
    // focus cycle root & focus-cycle-root's default Component is same.
    if (auto c = get_component_down_cycle(aComponent, FORWARD_TRAVERSAL); c and c != aComponent) {
      return c;
    }

    auto comp = aComponent;

    // See if the component is inside of policy provider.
    if (auto provider = get_topmost_provider(aContainer, comp)) {
      //TODO
      //log_platform_ln("### Asking FTP " + provider + " for component after " + comp);

      // FTP knows how to find component after the given. We don't.
      auto policy = provider->get_focus_traversal_policy();

      // Null result means that we overstepped the limit of the FTP's cycle.
      // In that case we must quit the cycle, otherwise return the component found.
      if (auto afterComp = policy->get_component_after(provider, comp)) {
        // TODO
        //log_platform_ln("### FTP returned " + afterComp);
        return afterComp;
      }

      comp = provider;
    }

    auto cycle = get_focus_traversal_cycle(aContainer);

    // TODO
    //log_platform_ln("### Cycle is " + cycle + ", component is " + comp);

    auto index = get_component_index(cycle, comp);

    if (index == -1U) {
      // TODO
      //log_platform_ln("### Didn't find component " + comp + " in a cycle " + aContainer);
      return get_first_component(aContainer);
    }

    for (++index; index != cycle.size(); ++index) {
      auto &&c = cycle[index];
      if (accept(c)) {
        return c;
      } else if ((c = get_component_down_cycle(c, FORWARD_TRAVERSAL))) {
        return c;
      }
    }

    if (aContainer->is_focus_cycle_root()) {
      this->cachedRoot = aContainer;
      this->cachedCycle = cycle;

      auto c = get_first_component(aContainer);

      this->cachedRoot = nullptr;
      this->cachedCycle.clear();

      return c;
    }

    return nullptr;
  });
}

std::shared_ptr<Component> ContainerOrderFocusTraversalPolicy::get_component_before(const std::shared_ptr<Component> &aContainer, const std::shared_ptr<Component> &aComponent) const {
  if (not aContainer or not aComponent) {
    throw std::runtime_error("aContainer and aComponent cannot be null");
  }
  if (not aContainer->is_focus_traversal_policy_provider() and not aContainer->is_focus_cycle_root()) {
    throw std::runtime_error("aContainer should be focus cycle root or focus traversal policy provider");

  } else if (aContainer->is_focus_cycle_root() and not aComponent->is_focus_cycle_root(aContainer)) {
    throw std::runtime_error("aContainer is not a focus cycle root of aComponent");
  }

  return aContainer->with_tree_locked([&]() -> std::shared_ptr<Component> {
    if (not (aContainer->is_visible() and aContainer->is_displayable())) {
      return nullptr;
    }

    auto comp = aComponent;

    // See if the component is inside of policy provider.
    if (auto provider = get_topmost_provider(aContainer, comp)) {
      // TODO
      //log_platform_ln("### Asking FTP " + provider + " for component after " + comp);

      // FTP knows how to find component after the given. We don't.
      auto policy = provider->get_focus_traversal_policy();
      auto beforeComp = policy->get_component_before(provider, comp);

      // Null result means that we overstepped the limit of the FTP's cycle.
      // In that case we must quit the cycle, otherwise return the component found.
      if (beforeComp) {
        // TODO
        // log_platform_ln("### FTP returned " + beforeComp);
        return beforeComp;
      }
      comp = provider;

      // If the provider is traversable it's returned.
      if (accept(comp)) {
        return comp;
      }
    }

    auto cycle = get_focus_traversal_cycle(aContainer);

    // TODO
    //log_platform_ln("### Cycle is " + cycle + ", component is " + comp);

    int index = get_component_index(cycle, comp);

    if (index == -1) {
      //TODO
      //log_platform_ln("### Didn't find component " + comp + " in a cycle " + aContainer);
      return get_last_component(aContainer);
    }

    for (--index; index >= 0; --index) {
      auto &&comp = cycle[index];
      auto tryComp = std::shared_ptr<Component> { };
      if (comp != aContainer and (tryComp = get_component_down_cycle(comp, BACKWARD_TRAVERSAL))) {
        return tryComp;
      } else if (accept(comp)) {
        return comp;
      }
    }

    if (aContainer->is_focus_cycle_root()) {
      this->cachedRoot = aContainer;
      this->cachedCycle = cycle;

      auto comp = get_last_component(aContainer);

      this->cachedRoot = nullptr;
      this->cachedCycle.clear();

      return comp;
    }
    return nullptr;
  });
}

std::shared_ptr<Component> ContainerOrderFocusTraversalPolicy::get_first_component(const std::shared_ptr<Component> &aContainer) const {
  // TODO
//  log_platform_ln("### Getting first component in " << aContainer);

  if (not aContainer) {
    throw std::runtime_error("aContainer cannot be null");
  }

  return aContainer->with_tree_locked([&]() -> std::shared_ptr<Component> {

    if (not (aContainer->is_visible() and aContainer->is_displayable())) {
      return nullptr;
    }

    auto cycle = this->cachedRoot == aContainer ? this->cachedCycle : get_focus_traversal_cycle(aContainer);
    if (cycle.empty()) {
      log_platform_ln("### Cycle is empty");
      return nullptr;
    }

    // TODO
//    log_platform_ln("### Cycle is " << cycle);

    for (auto &&comp : cycle) {
      if (accept(comp)) {
        return comp;
      } else if (comp != aContainer) {
        if (auto c = get_component_down_cycle(comp, FORWARD_TRAVERSAL)) {
          return c;
        }
      }
    }

    return nullptr;
  });
}

std::shared_ptr<Component> ContainerOrderFocusTraversalPolicy::get_last_component(const std::shared_ptr<Component> &aContainer) const {
  // TODO
  //log_platform_ln("### Getting last component in " << aContainer);

  if (not aContainer) {
    throw std::runtime_error("aContainer cannot be null");
  }

  return aContainer->with_tree_locked([&]() -> std::shared_ptr<Component> {
    if (not (aContainer->is_visible() and aContainer->is_displayable())) {
      return nullptr;
    }

    auto cycle = this->cachedRoot == aContainer ? this->cachedCycle : get_focus_traversal_cycle(aContainer);

    if (cycle.empty()) {
      log_platform_ln("### Cycle is empty");
      return nullptr;
    }

    // TODO
    //log_platform_ln("### Cycle is " + cycle);

    for (auto i = (int) cycle.size() - 1; i >= 0; --i) {
      auto comp = cycle[i];
      if (accept(comp)) {
        return comp;
      } else if (comp != aContainer) {
        if (comp->is_focus_traversal_policy_provider()) {
          if (auto retComp = comp->get_focus_traversal_policy()->get_last_component(comp)) {
            return retComp;
          }
        }
      }
    }
    return nullptr;
  });
}

std::shared_ptr<Component> ContainerOrderFocusTraversalPolicy::get_default_component(const std::shared_ptr<Component> &container) const {
  return get_first_component(container);
}

}
