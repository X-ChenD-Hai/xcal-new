#include <gtest/gtest.h>

#include <type_traits>

#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/markers.hpp"
using namespace xc::traits;
using namespace xc::ecs;

using namespace xc;
TEST(Ecs2, traits) {
    namespace tr = xc::traits;

    using vr1 = tr::value_record<1, 3, 5>;
    using vr2 = tr::value_record<4, 5, 6, 7, 8>;
    using vr3 = tr::value_record<>;
    using vr4 = tr::value_record<2, 1, 3, 4, 23, 455, 5>;
    using k1 = tr::as_integral_type_record_t<vr1>;
    using k2 = tr::as_integral_type_record_t<vr2>;
    using k3 = tr::as_integral_type_record_t<vr3>;
    using k4 = tr::as_integral_type_record_t<vr4>;
    using c1 = tr::order_preserving_merge_t<k1, k2>;
    using cc1 = tr::as_value_record_t<c1>;
    static_assert(
        std::is_same_v<xc::traits::value_record<1, 3, 4, 5, 5, 6, 7, 8>, cc1>);
    using sss1 = tr::sort_t<vr4>;
    static_assert(
        std::is_same_v<xc::traits::value_record<1, 2, 3, 4, 5, 23, 455>, sss1>);
    using sss2 = tr::sort_t<vr4, tr::greater_then>;
    static_assert(
        std::is_same_v<xc::traits::value_record<455, 23, 5, 4, 3, 2, 1>, sss2>);
}

TEST(Ecs2, markers2) {
    using makers = type_record<Optional<double>, ReadWrite<int>, Read<>,
                               Read<double>, int, Read<float>>;

    using t = collect_marker_t<Read, makers>;
    using et = collect_marker_t<ExcludeAll, makers>;
    using t2 = collect_default_marker_t<ExcludeAll, makers>;
    using dt = collect_default_marker_t<Read, makers>;
    using maker2 = type_record<long>;
    using dt2 = collect_default_marker_t<Read, maker2>;
}
TEST(Ecs2, markers) {
    using namespace xc::traits;
    using makers = type_record<Optional<double>, ReadWrite<int>, int,
                               Read<float>, Read<float>>;
    using smaekers = sort_markers_t<makers>;
}

template <typename... T>
using base_query_t = collect_all_markers_with_default_t<
    Read, template_record<ReadWrite, ExcludeAny, ExcludeAll>, T...>;
TEST(Ecs2_trait, comp) {
    using b = base_query_t<long>;
    static_assert(std::is_same_v<b, type_record<Read<long>, ReadWrite<>,
                                                ExcludeAny<>, ExcludeAll<>>>);

    using a = type_record<int, double, float>;
    using k = transform_if_t<a, same_as<int>, transfer_to<long>>;
    static_assert(std::is_same_v<type_record<long, double, float>, k>);
    constexpr auto k1 = count_if_v<a, same_as<int>>;
    static_assert(k1 == 1);

    using overload_set = decltype([](auto& a) {});
    using fn = decltype([](int& a) {});
}
