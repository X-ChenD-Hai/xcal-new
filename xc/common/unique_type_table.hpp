#pragma once
#include <vector>
template <typename Catgory, typename T>
class UniqueTypeTable {
    static std::vector<T> data_;
    static size_t next_id_;
    static T default_value_;

   public:
    template <typename Class>
    inline static size_t type_id() {
        static size_t id = []() {
            if (next_id_ + 1 >= data_.size()) {
                data_.resize(next_id_ + 1, default_value_);
            }
            return next_id_++;
        }();
        return id;
    }
    template <typename Class>
    inline static T& data() {
        return data_[type_id<Class>()];
    }
    inline static size_t count() { return next_id_; }

    inline static T& default_data() { return default_value_; }
    inline static const size_t& next_id() { return next_id_; }
};
template <typename Catgory, typename T>
T UniqueTypeTableDefaultData = T{};

template <typename Catgory, typename T>
size_t UniqueTypeTable<Catgory, T>::next_id_{};
template <typename Catgory, typename T>
std::vector<T> UniqueTypeTable<Catgory, T>::data_{};
template <typename Catgory, typename T>
T UniqueTypeTable<Catgory, T>::default_value_{UniqueTypeTableDefaultData<Catgory, T>};
