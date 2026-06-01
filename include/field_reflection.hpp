#ifndef FIELD_REFLECTION_HPP
#define FIELD_REFLECTION_HPP

#include <nlohmann/json.hpp>
#include <string_view>
#include <tuple>
#include <type_traits>

// Describes one field: its JSON key and a pointer-to-member.
template <typename T, typename MemberT>
struct Field {
    std::string_view   name;
    MemberT T::*       ptr;
};

// Deduction guide so Field{"name", &Struct::member} just works.
template <typename T, typename MemberT>
Field(std::string_view, MemberT T::*) -> Field<T, MemberT>;

// Concept: T has a static `fields()` returning a tuple of Field<T, ...>.
template <typename T>
concept HasFields = requires { { T::fields() } -> std::same_as<decltype(T::fields())>; };

// Generic from_json: iterates fields(), reads each key, keeps default if absent.
template <HasFields T>
void from_json(nlohmann::json const& j, T& obj) {
    std::apply([&](auto const&... field) {
        ([&] {
            if (j.contains(field.name)) {
                j.at(std::string(field.name)).get_to(obj.*field.ptr);
            }
        }(), ...);
    }, T::fields());
}

// Generic to_json: iterates fields(), writes each key.
template <HasFields T>
void to_json(nlohmann::json& j, T const& obj) {
    std::apply([&](auto const&... field) {
        ([&] { j[std::string(field.name)] = obj.*field.ptr; }(), ...);
    }, T::fields());
}

#endif // FIELD_REFLECTION_HPP
