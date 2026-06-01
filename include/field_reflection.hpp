#ifndef FIELD_REFLECTION_HPP
#define FIELD_REFLECTION_HPP

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>

// Describes one field: its JSON key and a pointer-to-member.
// name must point to a string with static storage duration (e.g., a string literal).
// Passing a temporary std::string will cause a dangling std::string_view.
template <typename T, typename MemberT>
struct Field
{
    std::string_view name;
    MemberT T::* ptr;
};

// Deduction guide so Field{"name", &Struct::member} just works.
template <typename T, typename MemberT>
Field(std::string_view, MemberT T::*) -> Field<T, MemberT>;

namespace detail
{
    template <typename>
    struct is_field : std::false_type
    {
    };

    template <typename T, typename MemberT>
    struct is_field<Field<T, MemberT>> : std::true_type
    {
    };

    // Checks that fields() returns a tuple whose every element is a Field.
    template <typename T>
    constexpr bool has_valid_fields()
    {
        if constexpr (!requires { T::fields(); })
        {
            return false;
        }
        else
        {
            return []<std::size_t... Is>(std::index_sequence<Is...>)
            {
                using Tuple = decltype(T::fields());
                return (is_field<std::tuple_element_t<Is, Tuple>>::value && ...);
            }(std::make_index_sequence<std::tuple_size_v<decltype(T::fields())>>{});
        }
    }
} // namespace detail

template <typename T>
concept HasFields = detail::has_valid_fields<T>();

// CRTP base: injects ADL-visible from_json/to_json as friend functions,
// scoped to the derived type. Derived must define static constexpr fields().
template <typename Derived>
struct Reflectable
{
    friend void from_json(nlohmann::json const& j, Derived& obj)
    {
        std::apply([&](auto const&... field)
        {
            ([&]
            {
                if (j.contains(field.name))
                {
                    try
                    {
                        j.at(std::string(field.name)).get_to(obj.*field.ptr);
                    }
                    catch (nlohmann::json::exception const& e)
                    {
                        throw std::runtime_error(std::string("Field '") +
                            std::string(field.name) + "': " + e.what());
                    }
                }
                // absent keys keep their C++ default value
            }(), ...);
        }, Derived::fields());
    }

    friend void to_json(nlohmann::json& j, Derived const& obj)
    {
        std::apply([&](auto const&... field)
        {
            ([&] { j[std::string(field.name)] = obj.*field.ptr; }(), ...);
        }, Derived::fields());
    }
};

#endif // FIELD_REFLECTION_HPP
