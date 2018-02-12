#ifndef camp_tuple_HPP__
#define camp_tuple_HPP__

/*!
 * \file
 *
 * \brief   Exceptionally basic tuple for host-device support
 */

#include "camp/camp.hpp"

#include <iostream>
#include <type_traits>

namespace camp
{

template <typename... Rest>
struct tuple;

namespace internal
{
  template <class T>
  struct unwrap_refwrapper {
    using type = T;
  };

  template <class T>
  struct unwrap_refwrapper<std::reference_wrapper<T>> {
    using type = T&;
  };

  template <class T>
  using special_decay_t =
      typename unwrap_refwrapper<typename std::decay<T>::type>::type;
}  // namespace internal

template <typename... Args>
CAMP_HOST_DEVICE constexpr auto make_tuple(Args&&... args)
    -> tuple<internal::special_decay_t<Args>...>;

namespace internal
{
  template <camp::idx_t index, typename Type>
  struct tuple_storage {
    CAMP_HOST_DEVICE constexpr tuple_storage() : val(){};

    CAMP_SUPPRESS_HD_WARN
    CAMP_HOST_DEVICE constexpr tuple_storage(Type const& v) : val{v} {}

    CAMP_SUPPRESS_HD_WARN
    CAMP_HOST_DEVICE constexpr tuple_storage(Type&& v)
        : val{std::move(static_cast<Type>(v))}
    {
    }

    CAMP_HOST_DEVICE constexpr const Type& get_inner() const noexcept
    {
      return val;
    }

    CAMP_CONSTEXPR14
    CAMP_HOST_DEVICE
    Type& get_inner() noexcept { return val; }

  public:
    Type val;
  };

  template <typename Indices, typename Typelist, typename TypeMap>
  struct tuple_helper;

  template <>
  struct tuple_helper<camp::idx_seq<>, camp::list<>, camp::list<>> {
  };

  template <typename... Types, camp::idx_t... Indices, typename... MapTypes>
  struct tuple_helper<camp::idx_seq<Indices...>,
                      camp::list<Types...>,
                      camp::list<MapTypes...>>
      : public internal::tuple_storage<Indices, Types>... {
    using TMap = camp::list<camp::list<MapTypes, camp::num<Indices>>...>;

    CAMP_HOST_DEVICE constexpr tuple_helper() {}

    CAMP_HOST_DEVICE constexpr tuple_helper(Types const&... args)
        : internal::tuple_storage<Indices, Types>(args)...
    {
    }

    CAMP_HOST_DEVICE constexpr tuple_helper(const tuple_helper& rhs)
        : tuple_storage<Indices, Types>(
              rhs.tuple_storage<Indices, Types>::get_inner())...
    {
    }


    template <typename... RTypes>
    CAMP_HOST_DEVICE tuple_helper& operator=(
        const tuple_helper<camp::idx_seq<Indices...>, RTypes...>& rhs)
    {
      return (camp::sink(
                  (this->tuple_storage<Indices, Types>::get_inner() =
                       rhs.tuple_storage<Indices, RTypes>::get_inner())...),
              *this);
    }
  };
}  // namespace internal

template <typename T, camp::idx_t I>
using tpl_get_ret = camp::at_v<typename T::TList, I>;
template <typename T, camp::idx_t I>
using tpl_get_store = internal::tuple_storage<I, tpl_get_ret<T, I>>;

template <typename... Elements>
struct tuple : public internal::tuple_helper<
                   typename camp::make_idx_seq<sizeof...(Elements)>::type,
                   camp::list<Elements...>,
                   camp::list<Elements...>> {
  using TList = camp::list<Elements...>;
  using type = tuple;

private:
  using Self = tuple;
  using Base = internal::
      tuple_helper<camp::make_idx_seq_t<sizeof...(Elements)>, TList, TList>;

public:
  // Constructors
  CAMP_HOST_DEVICE constexpr tuple() : Base{} {};
  CAMP_HOST_DEVICE constexpr tuple(tuple const& o)
      : Base(static_cast<Base const&>(o))
  {
  }
  CAMP_HOST_DEVICE constexpr tuple(tuple&& o)
      : Base{std::move(static_cast<Base>(o))}
  {
  }
  CAMP_HOST_DEVICE tuple& operator=(tuple const& rhs)
  {
    Base::operator=(static_cast<Base const&>(rhs.base));
  }
  CAMP_HOST_DEVICE tuple& operator=(tuple&& rhs)
  {
    Base::operator=(std::move(static_cast<Base>(rhs)));
  }

  template <typename... OtherTypes>
  CAMP_HOST_DEVICE constexpr explicit tuple(OtherTypes&&... rest)
      : Base{std::forward<OtherTypes>(rest)...}
  {
  }

  template <typename... RTypes>
  CAMP_HOST_DEVICE CAMP_CONSTEXPR14 Self& operator=(const tuple<RTypes...>& rhs)
  {
    Base::operator=(rhs);
    return *this;
  }

};

template <typename TagList, typename... Elements>
struct tagged_tuple
    : public internal::tuple_helper<
          typename camp::make_idx_seq<sizeof...(Elements)>::type,
          camp::list<Elements...>,
          TagList> {
  using TList = camp::list<Elements...>;
  using type = tagged_tuple;

private:
  using Self = tagged_tuple;
  using Base = internal::
      tuple_helper<camp::make_idx_seq_t<sizeof...(Elements)>, TList, TagList>;

public:
  // Constructors
  CAMP_HOST_DEVICE constexpr tagged_tuple() : Base{} {};
  CAMP_HOST_DEVICE constexpr tagged_tuple(tagged_tuple const& o)
      : Base(static_cast<Base const&>(o))
  {
  }
  CAMP_HOST_DEVICE constexpr tagged_tuple(tagged_tuple&& o)
      : Base{std::move(static_cast<Base>(o))}
  {
  }
  CAMP_HOST_DEVICE tagged_tuple& operator=(tagged_tuple const& rhs)
  {
    Base::operator=(static_cast<Base const&>(rhs.base));
  }
  CAMP_HOST_DEVICE tagged_tuple& operator=(tagged_tuple&& rhs)
  {
    Base::operator=(std::move(static_cast<Base>(rhs)));
  }

  template <typename... OtherTypes>
  CAMP_HOST_DEVICE constexpr explicit tagged_tuple(OtherTypes&&... rest)
      : Base{std::forward<OtherTypes>(rest)...}
  {
  }

  template <typename... RTypes>
  CAMP_HOST_DEVICE CAMP_CONSTEXPR14 Self& operator=(
      const tagged_tuple<RTypes...>& rhs)
  {
    Base::operator=(rhs);
    return *this;
  }
};

template <camp::idx_t i, typename T>
struct tuple_element;
template <camp::idx_t i, typename... Types>
struct tuple_element<i, tuple<Types...>> {
  using type = camp::at_v<typename tuple<Types...>::TList, i>;
};
template <camp::idx_t i, typename TypeMap, typename... Types>
struct tuple_element<i, tagged_tuple<TypeMap, Types...>> {
  using type = camp::at_v<typename tagged_tuple<TypeMap, Types...>::TList, i>;
};
template <camp::idx_t i, typename T>
using tuple_element_t = typename tuple_element<i, T>::type;

// by index
template <int index, typename... Args, template <typename...> class Tuple>
CAMP_HOST_DEVICE constexpr auto get(const Tuple<Args...>& t) noexcept
    -> tpl_get_ret<Tuple<Args...>, index> const&
{
  static_assert(sizeof...(Args) > index, "index out of range");
  return t.tpl_get_store<Tuple<Args...>, index>::get_inner();
}

template <int index, typename... Args, template <typename...> class Tuple>
CAMP_HOST_DEVICE constexpr auto get(Tuple<Args...>& t) noexcept
    -> tpl_get_ret<Tuple<Args...>, index>&
{
  static_assert(sizeof...(Args) > index, "index out of range");
  return t.tpl_get_store<Tuple<Args...>, index>::get_inner();
}

// by type
template <typename T, typename... Args, template <typename...> class Tuple>
CAMP_HOST_DEVICE constexpr auto get(const Tuple<Args...>& t) noexcept
    -> tpl_get_ret<Tuple<Args...>,
                   camp::at_key<typename Tuple<Args...>::TMap, T>::value> const&
{
  using index_type = camp::at_key<typename Tuple<Args...>::TMap, T>;
  static_assert(!std::is_same<camp::nil, index_type>::value,
                "invalid type index");
  return t.tpl_get_store<Tuple<Args...>, index_type::value>::get_inner();
}

template <typename T, typename... Args, template <typename...> class Tuple>
CAMP_HOST_DEVICE constexpr auto get(Tuple<Args...>& t) noexcept
    -> tpl_get_ret<Tuple<Args...>,
                   camp::at_key<typename Tuple<Args...>::TMap, T>::value>&
{
  using index_type = camp::at_key<typename Tuple<Args...>::TMap, T>;
  static_assert(!std::is_same<camp::nil, index_type>::value,
                "invalid type index");
  return t.tpl_get_store<Tuple<Args...>, index_type::value>::get_inner();
}

template <typename Tuple>
struct tuple_size;

template <typename... Args>
struct tuple_size<tuple<Args...>> {
  static constexpr size_t value = sizeof...(Args);
};

template <typename... Args>
struct tuple_size<tuple<Args...>&> {
  static constexpr size_t value = sizeof...(Args);
};

template <typename... Args>
struct tuple_size<tagged_tuple<Args...>> {
  static constexpr size_t value = sizeof...(Args);
};

template <typename... Args>
struct tuple_size<tagged_tuple<Args...>&> {
  static constexpr size_t value = sizeof...(Args);
};

template <typename... Args>
CAMP_HOST_DEVICE constexpr auto make_tuple(Args&&... args)
    -> tuple<internal::special_decay_t<Args>...>
{
  return tuple<internal::special_decay_t<Args>...>{std::forward<Args>(args)...};
}

template <typename TagList, typename... Args>
CAMP_HOST_DEVICE constexpr auto make_tagged_tuple(Args&&... args)
    -> tagged_tuple<internal::special_decay_t<Args>...>
{
  return tagged_tuple<TagList, internal::special_decay_t<Args>...>{
      std::forward<Args>(args)...};
}

template <typename... Args>
CAMP_HOST_DEVICE constexpr auto forward_as_tuple(Args&&... args) noexcept
    -> tuple<Args&&...>
{
  return tuple<Args&&...>(std::forward<Args>(args)...);
}

template <class... Types>
CAMP_HOST_DEVICE constexpr tuple<Types&...> tie(Types&... args) noexcept
{
  return tuple<Types&...>{args...};
}

template <typename... Lelem,
          typename... Relem,
          camp::idx_t... Lidx,
          camp::idx_t... Ridx>
CAMP_HOST_DEVICE constexpr auto tuple_cat_pair(tuple<Lelem...>&& l,
                                               camp::idx_seq<Lidx...>,
                                               tuple<Relem...>&& r,
                                               camp::idx_seq<Ridx...>) noexcept
    -> tuple<Lelem..., Relem...>
{
  return make_tuple(get<Lidx>(l)..., get<Ridx>(r)...);
}

CAMP_SUPPRESS_HD_WARN
template <typename Fn, camp::idx_t... Sequence, typename TupleLike>
CAMP_HOST_DEVICE constexpr auto invoke_with_order(TupleLike&& t,
                                                  Fn&& f,
                                                  camp::idx_seq<Sequence...>)
    -> decltype(f(get<Sequence>(t)...))
{
  return f(get<Sequence>(t)...);
}

CAMP_SUPPRESS_HD_WARN
template <typename Fn, typename TupleLike>
CAMP_HOST_DEVICE constexpr auto invoke(TupleLike&& t, Fn&& f) -> decltype(
    invoke_with_order(forward<TupleLike>(t),
                      forward<Fn>(f),
                      camp::make_idx_seq_t<tuple_size<TupleLike>::value>{}))
{
  return invoke_with_order(
      forward<TupleLike>(t),
      forward<Fn>(f),
      camp::make_idx_seq_t<tuple_size<TupleLike>::value>{});
}
}  // namespace camp

namespace internal
{
template <class Tuple, camp::idx_t... Idxs>
void print_tuple(std::ostream& os, Tuple const& t, camp::idx_seq<Idxs...>)
{
  camp::sink((void*)&(os << (Idxs == 0 ? "" : ", ") << camp::get<Idxs>(t))...);
}
}  // namespace internal

template <class... Args>
auto operator<<(std::ostream& os, camp::tuple<Args...> const& t)
    -> std::ostream&
{
  os << "(";
  internal::print_tuple(os, t, camp::make_idx_seq_t<sizeof...(Args)>{});
  return os << ")";
}


#endif /* camp_tuple_HPP__ */
