
    #include <memory>
    #include <type_traits>
    #include <utility>
    
    template <class T>
    struct wrapper_natif {
      T data;
    
    public:
      wrapper_natif() = default;
    
      template <typename... Args>
      wrapper_natif(Args &&...args) : data(std::forward<Args>(args)...) {}
    
      T &operator=(const T &other) {
        data = other;
        return data;
      }
    
      T &operator=(T &&other) {
        data = std::move(other);
        return data;
      }
    
      T &operator()() { return data; }
    
      const T &operator()() const { return data; }
    
      T &operator*() { return data; }
    
      const T &operator->() const { return data; }
    };
    
    template <class T> struct wrapper_construct : public T {
      using T::T;
    
      T &operator*() { return *this; }
    
      const T &operator->() const { return *this; }
    };
    
    template <typename T> struct wrapper {
      // If T is default constructible, type = T
      // Otherwise, type = std::unique_ptr<T>
      using type = std::conditional_t<
          std::is_fundamental_v<T>, wrapper_natif<T>,
          std::conditional_t<std::is_default_constructible_v<T>,
                             wrapper_construct<T>, std::unique_ptr<T>>>;
    };
    
    // Helper alias to avoid writing "typename wrapper<T>::type" everywhere
    template <typename T> using wrapper_t = typename wrapper<T>::type;
    
    // 2. The build_wrapper function that constructs the appropriate type.
    template <typename T, typename... Args>
    wrapper_t<T> build_wrapper(Args &&...args) {
      // If T is default constructible, just call its constructor
      if constexpr (std::is_fundamental_v<T>) {
        return wrapper_natif<T>(std::forward<Args>(args)...);
      } else if constexpr (std::is_default_constructible_v<T>) {
        return wrapper_construct<T>(std::forward<Args>(args)...);
      } else {
        // Otherwise, build a std::unique_ptr<T>
        return std::make_unique<T>(std::forward<Args>(args)...);
      }
    }
    