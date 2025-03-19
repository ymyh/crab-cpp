export module crab_cpp:panic;
import std;

export namespace crab_cpp
{
    /**
     * Prints a message then call std::abort.
     */
    template<typename ...Types>
    [[noreturn]] auto panic(std::format_string<Types...> format, Types&& ...args) -> void
    {
        std::println(std::cerr, "Panic encountered: ");
        std::println(std::cerr, format, std::forward<Types>(args)...);

        #ifdef CRAB_CPP_ENABLE_BACKTRACE
        std::println(std::cerr, "{}", std::stacktrace::current());
        #endif

        std::abort();
    }

    /**
     * Prints a message then call std::abort.
     */
    [[noreturn]] auto panic(std::string_view str) -> void
    {
        std::println(std::cerr, "Panic encountered: {}", str);

        #ifdef CRAB_CPP_ENABLE_BACKTRACE
        std::println(std::cerr, "{}", std::stacktrace::current());
        #endif

        std::abort();
    }

    /**
     * Prints a message then call std::abort.
     */
    template<typename ...Types>
    [[noreturn]] auto unimplemented(std::format_string<Types...> format, Types&& ...args) -> void
    {
       std::println(std::cerr, "Unimplemented yet: ");
       std::println(std::cerr, format, std::forward<Types>(args)...);
       std::abort();
    }

    /**
     * Prints a message then call std::abort.
     */
    [[noreturn]] auto unimplemented(std::string_view str) -> void
    {
        std::println(std::cerr, "Unimplemented yet: {}", str);
        std::abort();
    }
}
