export module crab_cpp:mutex;

import std;
import :option;

export namespace crab_cpp
{
    template<typename T>
    class MutexGuard;

    /**
     * @brief An owning mutual-exclusion primitive modelled after Rust's
     *        std::sync::Mutex
     * @tparam T The type of the protected value
     * @note The value can only be reached through a MutexGuard, which releases
     *       the lock when it is destroyed, including during stack unwinding
     * @note This type does not implement Rust's poisoning; see
     *       `.agents/design.md` for the rationale
     */
    template<typename T>
    class Mutex
    {
    private:
        mutable std::mutex m_mtx;
        T m_data;

    public:
        /**
         * @brief Constructs a mutex that owns the given value
         * @param value The value to protect
         */
        explicit Mutex(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_data(std::move(value))
        {

        }

        Mutex(const Mutex&) = delete;
        auto operator=(const Mutex&) -> Mutex& = delete;

        Mutex(Mutex&&) = delete;
        auto operator=(Mutex&&) -> Mutex& = delete;

        ~Mutex() = default;

        /**
         * @brief Acquires the lock, blocking until it becomes available
         * @return A guard that owns the lock and provides access to the value
         * @note Blocks the calling thread; deadlocks if the same thread already
         *       holds the lock
         * @warning The returned guard must not outlive this mutex
         */
        [[nodiscard]] auto lock() const -> MutexGuard<T>
        {
            return MutexGuard<T>{ std::unique_lock<std::mutex>{ m_mtx }, value_ptr() };
        }

        /**
         * @brief Attempts to acquire the lock without blocking
         * @return Some guard if the lock was acquired, None otherwise
         * @warning The returned guard must not outlive this mutex
         */
        [[nodiscard]] auto try_lock() const -> Option<MutexGuard<T>>
        {
            std::unique_lock<std::mutex> lock{ m_mtx, std::try_to_lock };

            if (!lock.owns_lock())
            {
                return None{};
            }

            return MutexGuard<T>{ std::move(lock), value_ptr() };
        }

        /**
         * @brief Returns a reference to the protected value without locking
         * @tparam Self The deduced reference type of *this
         * @return Reference to the protected value
         * @note The caller must already have exclusive access to the value
         */
        template<typename Self>
            requires (!std::is_const_v<std::remove_reference_t<Self>>)
        [[nodiscard]] auto get_mut(this Self&& self) noexcept -> T&
        {
            return self.m_data;
        }

        /**
         * @brief Consumes the mutex and returns the protected value
         * @return The protected value
         */
        [[nodiscard]] auto into_inner() && noexcept(std::is_nothrow_move_constructible_v<T>) -> T
        {
            return std::move(m_data);
        }

    private:
        /**
         * @brief Returns a mutable pointer to the protected value
         * @return Pointer to the protected value
         * @note This is the C++ analogue of Rust's UnsafeCell: it is sound only
         *       because the caller holds the lock
         */
        auto value_ptr() const noexcept -> T*
        {
            return const_cast<T*>(std::addressof(m_data));
        }

        friend class MutexGuard<T>;
    };

    /**
     * @brief RAII handle that owns a mutex lock and provides access to the
     *        protected value
     * @tparam T The type of the protected value
     */
    template<typename T>
    class MutexGuard
    {
    private:
        std::unique_lock<std::mutex> m_lock;
        T* m_data;

    public:
        MutexGuard(const MutexGuard&) = delete;
        auto operator=(const MutexGuard&) -> MutexGuard& = delete;

        /**
         * @brief Move constructor, leaving the moved-from guard empty
         * @param other The guard to move from
         */
        MutexGuard(MutexGuard&& other) noexcept
            : m_lock(std::move(other.m_lock)),
              m_data(other.m_data)
        {
            other.m_data = nullptr;
        }

        /**
         * @brief Move assignment, leaving the moved-from guard empty
         * @param other The guard to move from
         * @return Reference to this guard
         */
        auto operator=(MutexGuard&& other) noexcept -> MutexGuard&
        {
            if (this != std::addressof(other))
            {
                m_lock = std::move(other.m_lock);
                m_data = other.m_data;
                other.m_data = nullptr;
            }

            return *this;
        }

        ~MutexGuard() = default;

        /**
         * @brief Returns a reference to the protected value
         * @return Reference to the protected value
         */
        [[nodiscard]] auto operator*() const noexcept -> T&
        {
            return *this->m_data;
        }

        /**
         * @brief Returns a pointer to the protected value
         * @return Pointer to the protected value
         */
        [[nodiscard]] auto operator->() const noexcept -> T*
        {
            return this->m_data;
        }

        /**
         * @brief Returns a pointer to the protected value
         * @return Pointer to the protected value
         */
        [[nodiscard]] auto get() const noexcept -> T*
        {
            return this->m_data;
        }

    private:
        friend class Mutex<T>;

        MutexGuard(std::unique_lock<std::mutex> lock, T* data) noexcept
            : m_lock(std::move(lock)),
              m_data(data)
        {

        }
    };
}
