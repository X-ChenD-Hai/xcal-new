#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <cassert>
#include <iostream>
#include <mutex>
#include <print>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef _DEBUG
#define ASSERT(msg)                                                 \
    do {                                                            \
        if (!(msg)) {                                               \
            std::cout << "Assertion failed: " << #msg << std::endl; \
        }                                                           \
        assert(msg);                                                \
    } while (0)
#else
#define ASSERT(msg)
#endif
using CompMask = std::bitset<64>;
enum class EventType : uint32_t {
    //
    EntityCreated = 1 << 0,
    EntityDestroyed,
    //
    ComponentAttached = 1 << 2,
    ComponentDetached,
    ComponentUpdated,
    //
    ResourceLoaded = 1 << 4,

    //
    All = ~(uint32_t(1 << 31)),
};
struct Event {
    EventType type_id : 31;
    bool ignored : 1;
    Event(EventType type_id, bool ignored)
        : type_id(type_id), ignored(ignored) {}
    // bool ()
};
class EventPublisher;
class EventListener {
   public:
    EventListener(const EventListener&) = default;
    EventListener(EventListener&&) = default;
    EventListener& operator=(const EventListener&) = default;
    EventListener& operator=(EventListener&&) = default;
    virtual void event(Event* event) = 0;
    EventListener();
    virtual ~EventListener();
};

class EventPublisher {
   public:
    friend class EventListener;
    friend class ApplicationLoader;

   private:
    static inline std::queue<std::unique_ptr<Event>> events_{};
    static inline std::unordered_set<EventListener*> listeners_{};
    static inline std::unordered_set<EventListener*> removed_listeners_{};
    static inline std::mutex listeners_mutex_{};
    static inline std::mutex removed_listeners_mutex_{};
    static inline std::mutex events_mutex_{};
    static inline std::mutex sub_events_mutex_{};
    static void subscribe(EventListener* listener) {
        std::lock_guard lock(listeners_mutex_);
        listeners_.insert(listener);
    }
    static void unsubscribe(EventListener* listener) {
        std::lock_guard lock(removed_listeners_mutex_);
        removed_listeners_.insert(listener);
    }
    static void flush() {
        std::unique_lock lock(sub_events_mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return;
        }
        std::unordered_set<EventListener*> listeners;
        {
            std::lock_guard lock1(listeners_mutex_);
            listeners.swap(listeners_);
            {
                std::lock_guard lock(removed_listeners_mutex_);
                for (auto& listener : removed_listeners_) {
                    listeners.erase(listener);
                }
                removed_listeners_.clear();
            }
        }
        std::queue<std::unique_ptr<Event>> events;
        {
            std::lock_guard lock(events_mutex_);
            events.swap(events_);
        }
        while (!events.empty()) {
            auto e = std::move(events.front());
            events.pop();
            if (!e) continue;
            for (auto& listener : listeners) {
                try {
                    listener->event(e.get());
                } catch (...) {
                    std::cerr << "Exception caught in event listener"
                              << std::endl;
                }
            }
        }
        {
            std::lock_guard lock(listeners_mutex_);
            if (!listeners_.empty())
                for (auto& listener : listeners_) {
                    listeners.insert(listener);
                }
            listeners_.swap(listeners);
        }
    }

    static void publish(std::unique_ptr<Event> event) {
        std::lock_guard lock(events_mutex_);
        events_.push(std::move(event));
    }

   public:
};
EventListener::EventListener() { EventPublisher::subscribe(this); };
EventListener::~EventListener() { EventPublisher::unsubscribe(this); };

class ApplicationLoader {
    friend class EventPublisher;
    size_t compoent_count_{0};

    ApplicationLoader() = default;
    ~ApplicationLoader() = default;

   public:
    size_t component_counter() { return compoent_count_++; }
    static ApplicationLoader* instance() {
        static ApplicationLoader instance;
        return &instance;
    }
    ApplicationLoader(const ApplicationLoader&) = delete;
    ApplicationLoader(ApplicationLoader&&) = delete;
    ApplicationLoader& operator=(const ApplicationLoader&) = delete;
    ApplicationLoader& operator=(ApplicationLoader&&) = delete;
};
#define app (ApplicationLoader::instance())
struct IndexPairHash {
    static std::hash<size_t> hh;
    std::size_t operator()(const std::pair<size_t, size_t>& pair) const {
        return hh(pair.first | (pair.second << 32));
    }
};

template <typename T = void>
struct Component {
    static_assert(
        std::is_same_v<T, void> ||
            (!std::is_pointer_v<T> && !std::is_reference_v<T> &&
             std::is_trivial<T>::value && std::is_standard_layout<T>::value &&
             (sizeof(std::conditional<std::is_void_v<T>, int, T>) <= 64)),
        "Component must be trivial and standard layout");

    static const inline size_t id = []() {
        if constexpr (std::is_same_v<T, void>) {
            return 0;
        } else {
            if constexpr (std::is_const_v<T> || std::is_volatile_v<T>)
                return Component<std::remove_cv_t<T>>::id;
            else
                return app->component_counter();
        }
    }();
    static const inline CompMask size = sizeof(T);
    static auto mask() {
        if constexpr (std::is_same_v<T, void>) {
            return CompMask(0);
        } else {
            return CompMask(1ULL << id);
        }
    }
};
struct Entity {
    using entity_t = uint64_t;
    using id_t = uint32_t;
    using version_t = uint32_t;
    using mask_t = CompMask;
    entity_t id;
    Entity(entity_t id) : id(id) {}
    Entity(id_t index, version_t version) {
        id = index | (entity_t)version << 32;
    }
    id_t index() const noexcept { return (id_t)id; }
    version_t version() const noexcept { return id >> 32; }
};
class ArchetypeInfo;
class Chunk {
    std::unique_ptr<std::byte[]> ptr_{nullptr};
    size_t size_{0};

   public:
    static constexpr size_t CHUNK_SIZE = 1024;
    Chunk(std::unique_ptr<std::byte[]>&& ptr, size_t size)
        : ptr_(std::move(ptr)), size_(size) {}
    bool full() { return size_ == CHUNK_SIZE; }
    size_t push() { return size_++; }
    size_t size() const noexcept { return size_; }

    ~Chunk() {}

    std::byte* operator[](size_t index) {
        return ptr_.get() + index;
    }  //   operator []
    const std::byte* operator[](size_t index) const {
        return ptr_.get() + index;
    }  //   operator []

    Chunk(const Chunk&) = delete;
    Chunk(Chunk&& o) {
        std::swap(ptr_, o.ptr_);
        std::swap(size_, o.size_);
    }
    Chunk& operator=(const Chunk&) = delete;
    Chunk& operator=(Chunk&& o) {
        std::swap(ptr_, o.ptr_);
        std::swap(size_, o.size_);
        return *this;
    }
};

struct EntityEvent : public Event {};

class ArchetypeInfo {
    friend class Chunk;
    friend class ComponentAccess;
    friend class Controller;
    const CompMask mask_{};
    const std::array<size_t, 64>* component_sizes_{nullptr};
    std::vector<Chunk> chunks_{};
    std::unordered_set<std::pair<size_t, size_t>, IndexPairHash>
        free_indices_{};
    std::vector<size_t> chunk_column_strides_{};
    std::vector<size_t> chunk_columm_sizes_{};
    std::array<size_t, 64> component_offsets_{SIZE_MAX};
    std::array<size_t, 64> component_strides_{SIZE_MAX};

   public:
    ArchetypeInfo(const ArchetypeInfo&) = delete;
    ArchetypeInfo(ArchetypeInfo&&) = default;
    ArchetypeInfo& operator=(const ArchetypeInfo&) = delete;
    ArchetypeInfo& operator=(ArchetypeInfo&&) = delete;
    ArchetypeInfo(CompMask mask, std::array<size_t, 64>* component_sizes)
        : mask_(mask), component_sizes_(component_sizes) {
        size_t stride = 0;
        for (size_t i = 0; i < 64; i++) {
            if (!mask.test(i)) continue;
            auto size = (*component_sizes_)[i] * Chunk::CHUNK_SIZE;
            component_offsets_[i] = stride;
            component_strides_[i] = (*component_sizes_)[i];
            chunk_column_strides_.emplace_back(stride);
            chunk_columm_sizes_.emplace_back(size);
            stride += size;
        }
    }
    Chunk& last_chunk() {
        if (chunks_.empty() || chunks_.back().full()) {
            return chunks_.emplace_back(
                std::make_unique<std::byte[]>(chunk_column_strides_.back() +
                                              chunk_columm_sizes_.back()),
                0);
        }
        return chunks_.back();
    }
    inline std::byte* cell(size_t chunk_index, size_t row_index,
                           size_t component_id) {
        ASSERT(chunk_index < chunks_.size() && "Invalid chunk index");
        ASSERT(row_index < Chunk::CHUNK_SIZE && "Invalid row index");
        ASSERT(component_id < 64 && "Invalid component id");
        ASSERT(component_offsets_[component_id] <
                   chunk_column_strides_.back() + chunk_columm_sizes_.back() &&
               "Invalid component id");
        return chunks_[chunk_index][component_offsets_[component_id]] +
               row_index * component_strides_[component_id];
    }
    inline const std::byte* cell(size_t chunk_index, size_t row_index,
                                 size_t component_id) const {
        ASSERT(chunk_index < chunks_.size() && "Invalid chunk index");
        ASSERT(row_index < Chunk::CHUNK_SIZE && "Invalid row index");
        ASSERT(component_id < 64 && "Invalid component id");
        ASSERT(component_offsets_[component_id] <
                   chunk_column_strides_.back() + chunk_columm_sizes_.back() &&
               "Invalid component id");
        return chunks_[chunk_index][component_offsets_[component_id]] +
               row_index * component_strides_[component_id];
    }
    inline CompMask mask() const { return mask_; }
    inline const std::vector<Chunk>& chunks() const { return chunks_; }
    inline const size_t chunks_count() const { return chunks_.size(); }
    inline void free(size_t chunk_index, size_t row_index) {
        free_indices_.emplace(chunk_index, row_index);
    }
    inline std::pair<size_t, size_t> allocate() {
        if (!free_indices_.empty()) {
            auto [chunk_index, row_index] =
                *free_indices_.erase(free_indices_.begin());
            return {chunk_index, row_index};
        }
        auto& chunk = last_chunk();
        auto index = chunk.push();
        return {chunks_.size() - 1, index};
    }
    template <typename... Cs>
    static CompMask mask() {
        return (Component<Cs>::mask() | ...);
    }
};
class ComponentAccess {
    friend class Controller;
    ArchetypeInfo* archetype_{nullptr};
    Chunk* chunk_{nullptr};
    CompMask mask_{};

   public:
    ComponentAccess(ArchetypeInfo* archetype, Chunk* chunk, CompMask mask)
        : archetype_(archetype), chunk_(chunk), mask_(mask) {}
    template <typename T>
    T* data() const {
        ASSERT(mask_.test(Component<T>::id) &&
               "Component not attached to entity");
        return reinterpret_cast<T*>(
            (*chunk_)[archetype_->component_offsets_[Component<T>::id]]);
    }
    size_t size() const { return chunk_->size(); }
};
struct EntityName {
    size_t name_id;
    size_t name_id1;
    size_t name_id2;
    size_t name_id3;
    size_t name_id4;
    size_t name_id5;
    size_t name_id6;
    size_t name_id7;
    size_t name_id0;
    size_t name_id11;
};
class World {
    friend class Controller;

    std::unordered_map<size_t, size_t> mask_to_archetype_index_;
    std::vector<Entity> entities_;
    std::vector<Entity::version_t> versions_;
    std::vector<size_t> free_indices_;
    std::vector<size_t> archetype_indices_;
    std::vector<size_t> chunck_indices_;
    std::vector<size_t> row_indices_;

    std::vector<std::unique_ptr<ArchetypeInfo>> archetypes_;
    std::array<size_t, 64> component_sizes_{SIZE_MAX};

   public:
    World() {}
    Entity create_entity() {
        if (free_indices_.empty()) {
            entities_.emplace_back(entities_.size(), 0);
            versions_.emplace_back(0);
            archetype_indices_.emplace_back(SIZE_MAX);
            chunck_indices_.emplace_back(SIZE_MAX);
            row_indices_.emplace_back(SIZE_MAX);
            return entities_.back();
        }
        const auto index = free_indices_.back();
        free_indices_.pop_back();
        auto version = versions_[index];
        entities_[index] = Entity(index, version);
        archetype_indices_[index] = SIZE_MAX;
        chunck_indices_[index] = SIZE_MAX;
        row_indices_[index] = SIZE_MAX;
        return entities_[index];
    }
    void destroy_entity(Entity entity) {
        ASSERT(entity.version() == versions_[entity.index()] &&
               "Invalid version");
        versions_[entity.index()]++;
        free_indices_.push_back(entity.index());
    }
    inline ArchetypeInfo& archetype_at(size_t index) const {
        return *archetypes_[index];
    }
    void attach_component(Entity entity, size_t component_id,
                          void* data = nullptr) {
        const auto& index = entity.index();
        ASSERT(versions_[index] == entity.version() && "Invalid version");
        if (auto archetype_index_ = archetype_indices_[index];
            index_valid(archetype_index_)) {
            auto& archetype = archetype_at(archetype_index_);
            ASSERT(!archetypes_[archetype_index_]->mask().test(component_id) &&
                   "Component already attached to entity");
            auto new_mask = archetype.mask() | mask_of(component_id);
            auto new_archetype_index = archetype_index_from(new_mask);
            if (new_archetype_index != archetype_index_) {
                auto& new_archetype = archetype_at(new_archetype_index);
                auto [new_chunk_index, new_row] = new_archetype.allocate();
                copy_row_data(archetype, chunck_indices_[index],
                              row_indices_[index], new_archetype,
                              new_chunk_index, new_row);
                archetype.free(chunck_indices_[index], row_indices_[index]);
                archetype_indices_[index] = new_archetype_index;
                chunck_indices_[index] = new_chunk_index;
                row_indices_[index] = new_row;
                if (data)
                    set_component_data(component_id, new_archetype_index,
                                       new_chunk_index, new_row, data);
            }
        } else {
            auto mask = CompMask(1ULL << component_id);
            archetype_index_ = archetype_index_from(mask);
            archetype_indices_[index] = archetype_index_;
        }
    }
    void set_components(Entity entity, CompMask mask, void** data = nullptr) {
        const auto& index = entity.index();
        ASSERT(versions_[index] == entity.version() && "Invalid version");
        auto archetype_index_ = archetype_indices_[index];
        if (index_valid(archetype_index_)) {
            archetype_at(archetype_index_)
                .free(chunck_indices_[index], row_indices_[index]);
        }
        archetype_index_ = archetype_index_from(mask);
        auto [chunk_index, row_index] =
            archetype_at(archetype_index_).allocate();
        archetype_indices_[index] = archetype_index_;
        chunck_indices_[index] = chunk_index;
        row_indices_[index] = row_index;
        if (data) {
            for (size_t i = 0; i < 64; ++i)
                if (mask.test(i) && data[i]) {
                    set_component_data(i, archetype_index_, chunk_index,
                                       row_index, data[i]);
                }
        }
    }

    size_t archetype_index_from(CompMask mask) {
        if (auto it = mask_to_archetype_index_.find(mask.to_ulong());
            it != mask_to_archetype_index_.end()) {
            return it->second;
        }
        auto index = archetypes_.size();
        archetypes_.emplace_back(
            std::make_unique<ArchetypeInfo>(mask, &component_sizes_));
        mask_to_archetype_index_[mask.to_ulong()] = index;
        return index;
    }
    inline void set_component_data(size_t component_id, size_t archetype_index,
                                   size_t chunk_index, size_t row_index,
                                   void* data) {
        ASSERT(component_id < 64 && "Invalid component id");
        ASSERT(component_sizes_[component_id] > 0 && "Invalid component size");
        ASSERT(data && "Invalid data pointer");
        std::memcpy(archetypes_[archetype_index]->cell(chunk_index, row_index,
                                                       component_id),
                    data, component_sizes_[component_id]);
    }
    inline bool index_valid(size_t index) const noexcept {
        return index != SIZE_MAX;
    }
    static inline CompMask mask_of(size_t component_id) {
        return CompMask(1ULL << component_id);
    }
    inline void copy_component_data(size_t component_id, ArchetypeInfo& src,
                                    size_t src_chunk_index,
                                    size_t src_row_index, ArchetypeInfo& dst,
                                    size_t dst_chunk_index,
                                    size_t dst_row_index) {
        std::memcpy(dst.cell(dst_chunk_index, dst_row_index, component_id),
                    src.cell(src_chunk_index, src_row_index, component_id),
                    component_sizes_[component_id]);
    }
    inline void copy_row_data(ArchetypeInfo& src, size_t src_chunk_index,
                              size_t src_row_index, ArchetypeInfo& dst,
                              size_t dst_chunk_index, size_t dst_row_index) {
        for (size_t i = 0; i < 64; ++i)
            if (src.mask().test(i) && dst.mask().test(i)) {
                copy_component_data(i, src, src_chunk_index, src_row_index, dst,
                                    dst_chunk_index, dst_row_index);
            }
    }
    template <class Cs>
    inline size_t attach_component(Entity entity, const Cs& component) {
        if (component_sizes_[Component<Cs>::id] == SIZE_MAX)
            component_sizes_[Component<Cs>::id] = sizeof(Cs);
        ASSERT(component_sizes_[Component<Cs>::id] == sizeof(Cs) &&
               "Invalid component size");
        attach_component(entity, Component<Cs>::id, &component);
    }
    template <typename... Cs>
    inline void set_components(Entity entity, const Cs&... components) {
        attach_components<Cs...>();
        auto msak = (Component<Cs>::mask() | ...);
        std::array<void*, 64> data{nullptr};
        ((data[Component<Cs>::id] = (void*)&components), ...);
        set_components(entity, msak, data.data());
    }
    template <typename... Cs>
    CompMask attach_components() {
        ((component_sizes_[Component<Cs>::id] = sizeof(Cs)), ...);
        return (Component<Cs>::mask() | ...);
    }
};
class System {
   public:
    System() = default;
    virtual void update(const ComponentAccess& access) = 0;
    virtual CompMask required_components() const = 0;
    virtual CompMask optional_components() const { return CompMask(); }
    virtual bool event() { return false; }
    virtual ~System() = default;
};
class Controller {
    std::vector<std::unique_ptr<System>> systems_{};
    std::unique_ptr<World> world_{nullptr};

   public:
    Controller() : world_(std::make_unique<World>()) {}
    World& world() { return *world_; }
    void add_system(std::unique_ptr<System> system) {
        systems_.emplace_back(std::move(system));
    }
    void update() {
        ComponentAccess access{nullptr, nullptr, CompMask()};
        for (auto& system : systems_) {
            access.mask_ = system->required_components();
            for (auto& arch : world_->archetypes_) {
                access.archetype_ = arch.get();
                if ((arch->mask() & access.mask_) == access.mask_) {
                    for (auto& chunk : arch->chunks_) {
                        access.chunk_ = &chunk;
                        system->update(access);
                    }
                }
            }
        }
    }

   public:
    ~Controller() = default;
};
class PrintComponent : public System {
    std::vector<std::string> names_;
    void update(const ComponentAccess& access) override {
        std::println("PrintComponent");
        auto names = access.data<EntityName>();
        for (size_t i = 0; i < access.size(); ++i) {
            if (names[i].name_id != 0) {
                std::cout << "Name: " << names_[names[i].name_id] << std::endl;
            } else {
                std::cout << "No name" << std::endl;
                names_.emplace_back(std::format("Name {}", names_.size()));
                names[i].name_id = names_.size() - 1;
            }
        }
    }
    virtual CompMask required_components() const override {
        return ArchetypeInfo::mask<EntityName>();
    }

   public:
    PrintComponent() { names_.emplace_back("No name"); };
    ~PrintComponent() override = default;
};
class TransformComponent {
    float x;
    float y;
    float z;
};
class RotationComponent {
    float x;
    float y;
    float z;
};
class ScaleComponent {
    float x;
    float y;
    float z;
};

TEST(Controller, Test) {
    Controller controller;
    auto& world = controller.world();
    auto mask = world.attach_components<TransformComponent, RotationComponent,
                                        ScaleComponent, EntityName>();
    world.set_components(world.create_entity(), mask);
    world.set_components(world.create_entity(), mask);
    world.set_components(world.create_entity(), mask);
    world.set_components(world.create_entity(), mask);
    world.set_components(world.create_entity(), mask);
    world.set_components(world.create_entity(), mask);
    controller.add_system(std::make_unique<PrintComponent>());
    controller.update();
    controller.update();
    controller.update();
}
