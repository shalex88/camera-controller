#pragma once

#include <variant>
#include <string>
#include <type_traits>
#include <utility>

// Helper type for void Results
struct Empty {};

template<typename T, typename E = std::string>
class Result {
public:
    // Success constructor - for non-void types
    template<typename U = T, typename = std::enable_if_t<!std::is_void_v<U>>>
    explicit Result(U value) : data_(std::move(value)) {}

    // Success constructor - for void type
    template<typename U = T, typename = std::enable_if_t<std::is_void_v<U>>>
    explicit Result(Empty = {}) : data_(Empty{}) {}

    // Error constructor
    explicit Result(E error) : data_(std::move(error)) {}

    // Check if the result contains a success value
    [[nodiscard]] bool isSuccess() const {
        if constexpr (std::is_void_v<T>) {
            return std::holds_alternative<Empty>(data_);
        } else {
            return std::holds_alternative<T>(data_);
        }
    }

    // Check if the result contains an error
    [[nodiscard]] bool isError() const {
        return std::holds_alternative<E>(data_);
    }

    // Get the success value. Throws if result contains error.
    // Only available for non-void Results
    template<typename U = T, typename = std::enable_if_t<!std::is_void_v<U>>>
    [[nodiscard]] const U& value() const& {
        return std::get<U>(data_);
    }

    // Get the success value. Throws if result contains error.
    // Only available for non-void Results
    template<typename U = T, typename = std::enable_if_t<!std::is_void_v<U>>>
    U&& value() && {
        return std::move(std::get<U>(data_));
    }

    // Get the error value. Throws if result contains success.
    [[nodiscard]] const E& error() const& {
        return std::get<E>(data_);
    }

    // Get the error value. Throws if result contains success.
    E&& error() && {
        return std::move(std::get<E>(data_));
    }

    // Convenience function to create a success result
    template<typename U = T>
    static Result<U, E> success(U value) {
        return Result<U, E>(std::move(value));
    }

    // Convenience function to create a void success result
    template<typename U = T, typename = std::enable_if_t<std::is_void_v<U>>>
    static Result<void, E> success() {
        return Result<void, E>(Empty{});
    }

    // Convenience function to create an error result
    static Result<T, E> error(E error) {
        return Result<T, E>(std::move(error));
    }

private:
    using DataType = std::conditional_t<std::is_void_v<T>,
                                      std::variant<Empty, E>,
                                      std::variant<T, E>>;
    DataType data_;
};
