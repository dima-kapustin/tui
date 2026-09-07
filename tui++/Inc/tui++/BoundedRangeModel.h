#pragma once

// Swing's BoundedRangeModel / DefaultBoundedRangeModel: the numeric model
// behind a scrollbar. The value stays within [minimum, maximum - extent];
// listeners are notified whenever any property changes.

#include <functional>
#include <vector>

namespace tui {

class BoundedRangeModel {
public:
  using Listener = std::function<void()>;

  virtual ~BoundedRangeModel() = default;

  virtual int get_value() const = 0;
  virtual int get_extent() const = 0;
  virtual int get_minimum() const = 0;
  virtual int get_maximum() const = 0;

  // The largest value is maximum - extent.
  virtual int get_maximum_value() const = 0;

  virtual void set_value(int value) = 0;
  virtual void set_extent(int extent) = 0;
  virtual void set_minimum(int minimum) = 0;
  virtual void set_maximum(int maximum) = 0;

  // Sets value/extent/minimum/maximum at once; one change notification fires
  // no matter how many properties actually changed.
  virtual void set_range_properties(int value, int extent, int minimum, int maximum) = 0;

  virtual void add_change_listener(Listener const &listener) = 0;
};

// The default implementation, enforcing Swing's invariants:
//   minimum <= value <= maximum - extent
// A property change that violates an invariant adjusts the other properties
// (e.g. lowering the maximum pulls the value down with it).
class DefaultBoundedRangeModel: public BoundedRangeModel {
  int value = 0;
  int extent = 0;
  int minimum = 0;
  int maximum = 100;

  std::vector<Listener> listeners;

  void notify_changed() {
    for (auto const &listener : this->listeners) {
      listener();
    }
  }

public:
  int get_value() const override {
    return this->value;
  }

  int get_extent() const override {
    return this->extent;
  }

  int get_minimum() const override {
    return this->minimum;
  }

  int get_maximum() const override {
    return this->maximum;
  }

  int get_maximum_value() const override {
    return this->maximum - this->extent;
  }

  void set_value(int value) override {
    set_range_properties(value, this->extent, this->minimum, this->maximum);
  }

  void set_extent(int extent) override {
    set_range_properties(this->value, extent, this->minimum, this->maximum);
  }

  void set_minimum(int minimum) override {
    set_range_properties(this->value, this->extent, minimum, this->maximum);
  }

  void set_maximum(int maximum) override {
    set_range_properties(this->value, this->extent, this->minimum, maximum);
  }

  void set_range_properties(int value, int extent, int minimum, int maximum) override {
    if (maximum < minimum) {
      maximum = minimum;
    }
    if (extent < 0) {
      extent = 0;
    }
    if (extent > maximum - minimum) {
      extent = maximum - minimum;
    }
    if (value < minimum) {
      value = minimum;
    }
    if (value > maximum - extent) {
      value = maximum - extent;
    }
    if (value == this->value and extent == this->extent and minimum == this->minimum and maximum == this->maximum) {
      return;
    }

    this->value = value;
    this->extent = extent;
    this->minimum = minimum;
    this->maximum = maximum;
    notify_changed();
  }

  void add_change_listener(Listener const &listener) override {
    this->listeners.emplace_back(listener);
  }
};

}
