#ifndef OPTIONAL_VALUE_H
#define OPTIONAL_VALUE_H

template<typename T>
class OptionalValue {
    bool has_value_;
    T value_;
public:
    OptionalValue() : has_value_(false) {}
    OptionalValue(T value) : has_value_(true), value_(value) {}
    bool has_value() const { return has_value_; }
    T value() const { return value_; }
    void reset() { has_value_ = false; }
};

#endif // OPTIONAL_VALUE_H