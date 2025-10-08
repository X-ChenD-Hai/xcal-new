// #include <stddef.h>

// #include <array>
// #include <bitset>
// #include <cassert>
// #include <typeindex>
// #include <vector>
// #include <vector>

// namespace xc {
// using Entity = std::uint64_t;
// using EntityVector = std::vector<Entity>;
// using VersionVector = std::vector<uint32_t>;
// using SizeType = std::uint32_t;
// inline Entity MakeEntity(uint32_t index, uint32_t version) noexcept {
//     return (static_cast<Entity>(version) << 32) | static_cast<Entity>(index);
// }
// inline uint32_t Index(Entity entity) noexcept {
//     return static_cast<uint32_t>(entity);
// }
// inline uint32_t Version(Entity entity) noexcept {
//     return static_cast<uint32_t>(entity >> 32);
// }
// template <typename T>
// struct Component {
//     static constexpr std::size_t ID() noexcept {
//         static const std::type_index idx = std::type_index(typeid(T));
//         return idx.hash_code();  // 运行期唯一，可换静态计数器
//     }
// };
// using CompMask = std::bitset<64>;

// template <class... Cs>
// constexpr CompMask MakeMask() noexcept {
//     CompMask m;
//     (m.set((Component<Cs>::ID() & 0xff)), ...);
//     return m;
// }

// class Chunk {
//     friend class World;

//    private:
//     std::byte* ptr{nullptr};
//     SizeType capacity = 0;
//     SizeType count = 0;
//     std::vector<std::size_t> offsets;
//     std::vector<std::size_t> sizes;

//    public:
//     Chunk(CompMask mask, const std::array<std::size_t, 64>& compSizes) {
//         std::size_t stride = 0;
//         for (size_t i = 0; i < 64; i++)
//             if (mask.test(i)) {
//                 offsets.emplace_back(stride);
//                 sizes.emplace_back(compSizes[i]);
//                 stride += compSizes[i] * 1024;
//             }
//         ptr = new std::byte[stride];
//         capacity = 1024;
//     }
//     ~Chunk() { delete[] ptr; }

//     void* Column(size_t i) { return ptr + offsets[i]; }
//     size_t Size(size_t i) const { return sizes[i]; }
//     size_t Capacity() const { return capacity; }
//     bool Full() const { return count == capacity; }

//     size_t Push() { return count++; }
// };

// class Archetype {
//     friend class World;

//    private:
//     CompMask mask;
//     std::array<std::size_t, 64> compSizes;
//     std::vector<Chunk> chunks;

//    public:
//     Archetype(CompMask mask, const std::array<std::size_t, 64>& compSizes)
//         : mask(mask), compSizes(compSizes) {}

//     Chunk& LastChunk() {
//         if (chunks.empty() || chunks.back().Full())
//             chunks.emplace_back(mask, compSizes);
//         return chunks.back();
//     }
//     CompMask Mask() const { return mask; }
// };

// class World {
//    private:
//     EntityVector entities;
//     VersionVector versions;
//     std::vector<SizeType> freeIndices;
//     std::vector<Archetype> archetypes;
//     std::vector<size_t> entityArchetypes;
//     std::vector<std::size_t> entityChunks;
//     std::vector<std::size_t> entityRow;

//    public:
//     World() = default;
//     Entity CreateEntity() {
//         uint32_t index;
//         if (!freeIndices.empty()) {
//             index = freeIndices.back();
//             freeIndices.pop_back();
//             versions[index] = 1;
//             entityArchetypes[index] = SIZE_MAX;
//             entityChunks[index] = SIZE_MAX;
//             entityRow[index] = SIZE_MAX;
//             entities[index] = MakeEntity(index, 1);
//         } else {
//             index = static_cast<uint32_t>(entities.size());
//             versions.emplace_back(1);
//             entityArchetypes.emplace_back(SIZE_MAX);
//             entityChunks.emplace_back(SIZE_MAX);
//             entityRow.emplace_back(SIZE_MAX);
//             entities.emplace_back(MakeEntity(index, 1));
//         }
//         return entities[index];
//     }
//     void DestroyEntity(Entity entity) {
//         SizeType idx = Index(entity);
//         assert(idx < entities.size() && Version(entity) == versions[idx]);
//         versions[idx]++;
//         freeIndices.push_back(idx);
//     }
//     template <typename C>
//     void AddComponent(Entity entity, const C& component) {
//         SizeType idx = Index(entity);
//         assert(idx < entities.size() && Version(entity) == versions[idx]);
//         CompMask oldMask;
//         if (entityArchetypes[idx] != SIZE_MAX)
//             oldMask = archetypes[entityArchetypes[idx]].Mask();
//         CompMask newMask = oldMask;
//         newMask.set(Component<C>::ID() & 0xff);
//         size_t archetypeIdx = SIZE_MAX;
//         for (size_t i = 0; i < archetypes.size(); i++)
//             if (archetypes[i].Mask() == newMask) {
//                 archetypeIdx = i;
//                 break;
//             }
//         if (archetypeIdx == SIZE_MAX) {
//             std::array<size_t, 64> compSizes;
//             for (size_t i = 0; i < 64; i++)
//                 if (newMask.test(i)) {
//                 }
//         }
//     }

//     template <typename... Cs, typename F>
//     void ForEach(F&& f) {
//         CompMask targetMask = MakeMask<Cs...>();
//         for (size_t i = 0; i < archetypes.size(); i++) {
//             if ((archetypes[i].Mask() & targetMask) == targetMask) {
//                 for (size_t j = 0; j < archetypes[i].chunks.size(); j++) {
//                     Chunk& chunk = archetypes[i].chunks[j];
//                     for (size_t k = 0; k < chunk.count; k++) {
//                         f(chunk.Column(0), chunk.Size(0));
//                     }
//                 }
//             }
//         }
//     }
// };
// }  // namespace xc
