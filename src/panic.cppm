export module crab_cpp:panic;
import std;

export namespace crab_cpp
{
    /**
     * Prints a message then call std::terminate.
     */
    template<typename ...Types>
    [[noreturn]] auto panic() noexcept -> void
    {
        std::println(std::cerr, "Panic encountered.");

        #ifdef CRAB_CPP_ENABLE_BACKTRACE
            std::println(std::cerr, "{}", std::stacktrace::current());
        #endif

        std::terminate();
    }

    /**
     * Prints a message then call std::terminate.
     */
    template<typename ...Types>
    [[noreturn]] auto panic(std::format_string<Types...> format, Types&& ...args) noexcept -> void
    {
        std::print(std::cerr, "Panic encountered: ");
        std::println(std::cerr, format, std::forward<Types>(args)...);

        #ifdef CRAB_CPP_ENABLE_BACKTRACE
            std::println(std::cerr, "{}", std::stacktrace::current());
        #endif

        std::terminate();
    }

    /**
     * Prints a message then call std::terminate.
     */
    [[noreturn]] auto panic(std::string_view str) -> void
    {
        std::println(std::cerr, "Panic encountered: {}", str);

        #ifdef CRAB_CPP_ENABLE_BACKTRACE
            std::println(std::cerr, "{}", std::stacktrace::current());
        #endif

        std::terminate();
    }

    /**
     * Prints a message then call panic.
     */
    template<typename ...Types>
    [[noreturn]] auto unimplemented() -> void
    {
       std::println(std::cerr, "Unimplemented yet.");
       panic();
    }

    /**
     * Prints a message then call panic.
     */
    template<typename ...Types>
    [[noreturn]] auto unimplemented(std::format_string<Types...> format, Types&& ...args) -> void
    {
       std::println(std::cerr, "Unimplemented yet.");
       panic(format, std::forward<Types>(args)...);
    }

    /**
     * Prints a message then call panic.
     */
    [[noreturn]] auto unimplemented(std::string_view str) -> void
    {
        std::println(std::cerr, "Unimplemented yet.");
        panic(str);
    }
}
