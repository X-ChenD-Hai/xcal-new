#include <corecrt.h>

#include <cstddef>
#include <format>
#include <memory>
#include <print>
#include <source_location>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xc {

class Logger : public std::enable_shared_from_this<Logger> {
   public:
    template <typename... Args>
    void log(const std::format_string<Args...> fmt, std::source_location loc,
             size_t level, Args&&... args) {
        auto msg = std::format(fmt, std::forward<Args>(args)...);
        std::println("[{}]<{}>[{}:{}] => {}", level_map_of(level), name_,
                     loc.file_name(), loc.line(), msg);
    }

    struct LoggerContext {
        Logger* logger;
        std::source_location loc;
        std::size_t level;
        template <typename... Args>
        void operator()(const std::format_string<Args...> fmt,
                        Args&&... args) && {
            logger->log(fmt, loc, level, std::forward<Args>(args)...);
        }
    };
    LoggerContext operator[](
        std::size_t level,
        std::source_location loc = std::source_location::current()) {
        return {this, loc, level};
    }

    template <typename T = void>
    static constexpr std::string_view name_of() {
        return typeid(T).name();
    }
    template <typename T = void>
    static Logger& instence() {
        static Logger instance(name_of<T>());
        return instance;
    }
    static Logger& instence(
        std::string_view name =
            std::source_location::current().function_name()) {
        static std::unordered_map<std::string_view, std::unique_ptr<Logger>>
            instances;
        if (instances.find(name) == instances.end()) {
            auto n = new Logger(name);
            instances[name] = std::unique_ptr<Logger>{n};
        }
        return *instances[name];
    }
    static std::string_view level_map_of(size_t i) {
        if (i < level_map.size()) {
            return level_map[i];
        }
        return "UNKNOWN";
    }
    ~Logger() = default;
    Logger() = delete;

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

   protected:
    Logger(std::string_view name) : name_(name) {}

   private:
    std::string_view name_{};

   private:
    static std::vector<std::string_view> level_map;
};

inline std::vector<std::string_view> Logger::level_map = {
    "\033[94mDEBUG\033[0m", "\033[92mINFO\033[0m", "\033[93mWARN\033[0m",
    "\033[91mERROR\033[0m"};
enum class LogLevel : size_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
};
template <>
inline std::string_view Logger::name_of<void>() {
    return "root";
}
}  // namespace xc
#define XC_INIT_LOG(name) auto& xc_m_logger__ = ::xc::Logger::instence(name);
#define XC_LOG(level, ...) \
    xc_m_logger__[(size_t)(::xc::LogLevel::level)](__VA_ARGS__)
#define XC_TLOG(level, ...)                                           \
    ::xc::Logger::instence<std::remove_pointer_t<decltype(this)>>()[( \
        size_t)(xc::LogLevel::level)](__VA_ARGS__)
#define ON_DEBUG(...) __VA_ARGS__
#define XC_SELF_PTR ((void*)this)
