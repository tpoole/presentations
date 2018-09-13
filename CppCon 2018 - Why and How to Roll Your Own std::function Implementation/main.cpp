#include <benchmark/benchmark.h>

#include <functional>
#include <memory>
#include <array>

//=============================================================================
namespace inheritance_heap {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    template <typename Functor>
    function (Functor f)
        : functorHolderPtr (new FunctorHolder<Functor, Result, Arguments...> (f))
    {}

    function (const function& other)
    {
        if (other.functorHolderPtr != nullptr)
            functorHolderPtr = other.functorHolderPtr->clone();
    }

    function& operator= (function const& other)
    {
        delete functorHolderPtr;

        if (other.functorHolderPtr != nullptr)
            functorHolderPtr = other.functorHolderPtr->clone();

        return *this;
    }

    function() = default;

    ~function()
    {
        delete functorHolderPtr;
    }

    Result operator() (Arguments&&... args) const
    {
        return (*functorHolderPtr) (std::forward<Arguments> (args)...);
    }

private:
    template <typename ReturnType, typename... Args>
    struct FunctorHolderBase
    {
        virtual ~FunctorHolderBase() {}
        virtual ReturnType operator()(Args&&...) = 0;
        virtual FunctorHolderBase* clone() const = 0;
    };

    template <typename Functor, typename ReturnType, typename... Args>
    struct FunctorHolder final : FunctorHolderBase<Result, Arguments...>
    {
        FunctorHolder (Functor func) : f (func) {}

        ReturnType operator()(Args&&... args) override
        {
            return f (std::forward<Arguments> (args)...);
        }

        FunctorHolderBase<Result, Arguments...>* clone() const override
        {
            return new FunctorHolder (f);
        }

        Functor f;
    };

    FunctorHolderBase<Result, Arguments...>* functorHolderPtr = nullptr;
};

}

//=============================================================================
namespace inheritance_stack {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    template <typename Functor>
    function (Functor f)
    {
        static_assert (sizeof (FunctorHolder<Functor, Result, Arguments...>) <= sizeof (stack), "Too big!");
        functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack);
        new (functorHolderPtr) FunctorHolder<Functor, Result, Arguments...> (f);
    }

    function (const function& other)
    {
        if (other.functorHolderPtr != nullptr)
        {
            functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack);
            other.functorHolderPtr->copyInto (functorHolderPtr);
        }
    }

    function& operator= (function const& other)
    {
        if (functorHolderPtr != nullptr)
        {
            functorHolderPtr->~FunctorHolderBase<Result, Arguments...>();
            functorHolderPtr = nullptr;
        }

        if (other.functorHolderPtr != nullptr)
        {
            functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack);
            other.functorHolderPtr->copyInto (functorHolderPtr);
        }

        return *this;
    }

    function() = default;

    ~function()
    {
        if (functorHolderPtr != nullptr)
            functorHolderPtr->~FunctorHolderBase<Result, Arguments...>();
    }

    Result operator() (Arguments&&... args) const
    {
        return (*functorHolderPtr) (std::forward<Arguments> (args)...);
    }

private:
    template <typename ReturnType, typename... Args>
    struct FunctorHolderBase
    {
        virtual ~FunctorHolderBase() {}
        virtual ReturnType operator()(Args&&...) = 0;
        virtual void copyInto (void*) const = 0;
    };

    template <typename Functor, typename ReturnType, typename... Args>
    struct FunctorHolder final : FunctorHolderBase<Result, Arguments...>
    {
        FunctorHolder (Functor func) : f (func) {}

        ReturnType operator()(Args&&... args) override
        {
            return f (std::forward<Arguments> (args)...);
        }

        void copyInto (void* destination) const override
        {
            new (destination) FunctorHolder (f);
        }

        Functor f;
    };

    typename std::aligned_storage<32>::type stack;
    FunctorHolderBase<Result, Arguments...>* functorHolderPtr = nullptr;
};

}

//=============================================================================
namespace inheritance_stack_or_heap {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    template <typename Functor>
    function (Functor f)
    {
        if (sizeof (FunctorHolder<Functor, Result, Arguments...>) <= sizeof (stack))
        {
            functorHolderPtr = (decltype (functorHolderPtr)) std::addressof (stack);
            new (functorHolderPtr) FunctorHolder<Functor, Result, Arguments...> (f);
        }
        else
        {
            functorHolderPtr = new FunctorHolder<Functor, Result, Arguments...> (f);
        }
    }

    function (const function& other)
    {
        if (other.functorHolderPtr != nullptr)
        {
            if (other.functorHolderPtr == (decltype (other.functorHolderPtr)) std::addressof (other.stack))
            {
                functorHolderPtr = (decltype (functorHolderPtr)) std::addressof (stack);
                other.functorHolderPtr->copyInto (functorHolderPtr);
            }
            else
            {
                functorHolderPtr = other.functorHolderPtr->clone();
            }
        }
    }

    function& operator= (function const& other)
    {
        if (functorHolderPtr != nullptr)
        {
            if (functorHolderPtr == (decltype (functorHolderPtr)) std::addressof (stack))
                functorHolderPtr->~FunctorHolderBase();
            else
                delete functorHolderPtr;

            functorHolderPtr = nullptr;
        }

        if (other.functorHolderPtr != nullptr)
        {
            if (other.functorHolderPtr == (decltype (other.functorHolderPtr)) std::addressof (other.stack))
            {
                functorHolderPtr = (decltype (functorHolderPtr)) std::addressof (stack);
                other.functorHolderPtr->copyInto (functorHolderPtr);
            }
            else
            {
                functorHolderPtr = other.functorHolderPtr->clone();
            }
        }

        return *this;
    }

    function() = default;

    ~function()
    {
        if (functorHolderPtr == (decltype (functorHolderPtr)) std::addressof (stack))
            functorHolderPtr->~FunctorHolderBase();
        else
            delete functorHolderPtr;
    }

    Result operator() (Arguments&&... args) const
    {
        return (*functorHolderPtr) (std::forward<Arguments> (args)...);
    }

private:
    template <typename ReturnType, typename... Args>
    struct FunctorHolderBase
    {
        virtual ~FunctorHolderBase() {}
        virtual ReturnType operator()(Args&&...) = 0;
        virtual void copyInto (void*) const = 0;
        virtual FunctorHolderBase<Result, Arguments...>* clone() const = 0;
    };

    template <typename Functor, typename ReturnType, typename... Args>
    struct FunctorHolder final : FunctorHolderBase<Result, Arguments...>
    {
        FunctorHolder (Functor func) : f (func) {}

        ReturnType operator()(Args&&... args) override
        {
            return f (std::forward<Arguments> (args)...);
        }

        void copyInto (void* destination) const override
        {
            new (destination) FunctorHolder (f);
        }

        FunctorHolderBase<Result, Arguments...>* clone() const override
        {
            return new FunctorHolder (f);
        }

        Functor f;
    };

    typename std::aligned_storage<32>::type stack;
    FunctorHolderBase<Result, Arguments...>* functorHolderPtr = nullptr;
};

}

//=============================================================================
namespace pointer_heap {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    template <typename Functor>
    function (Functor f)
        : invokePtr  (reinterpret_cast<invokePtr_t>  (invoke<Functor>)),
          createPtr  (reinterpret_cast<createPtr_t>  (create<Functor>)),
          destroyPtr (reinterpret_cast<destroyPtr_t> (destroy<Functor>)),
          storageSize (sizeof (Functor)),
          storage (new char[storageSize])
    {
        createPtr (storage.get(), std::addressof (f));
    }

    function() = default;

    function (const function& other)
    {
        if (other.storage != nullptr)
        {
            invokePtr  = other.invokePtr;
            createPtr  = other.createPtr;
            destroyPtr = other.destroyPtr;

            storageSize = other.storageSize;
            storage.reset (new char[storageSize]);

            createPtr (storage.get(), other.storage.get());
        }
    }

    function& operator= (function const& other)
    {
        if (storage != nullptr)
        {
            destroyPtr (storage.get());
            storage.reset();
        }

        if (other.storage != nullptr)
        {
            invokePtr = other.invokePtr;
            createPtr = other.createPtr;
            destroyPtr = other.destroyPtr;

            storageSize = other.storageSize;
            storage.reset (new char[storageSize]);

            createPtr (storage.get(), other.storage.get());
        }

        return *this;
    }

    ~function()
    {
        if (storage != nullptr)
            destroyPtr (storage.get());
    }

    Result operator() (Arguments&&... args) const
    {
        return invokePtr (storage.get(), std::forward<Arguments> (args)...);
    }

private:
    template <typename Functor>
    static Result invoke (Functor* f, Arguments&&... args)
    {
        return (*f)(std::forward<Arguments> (args)...);
    }

    template <typename Functor>
    static void create (Functor* destination, Functor* source)
    {
        new (destination) Functor (*source);
    }

    template <typename Functor>
    static void destroy (Functor* f)
    {
        f->~Functor();
    }

    using invokePtr_t = Result(*)(void*, Arguments&&...);
    using createPtr_t = void(*)(void*, void*);
    using destroyPtr_t = void(*)(void*);

    invokePtr_t invokePtr;
    createPtr_t createPtr;
    destroyPtr_t destroyPtr;

    size_t storageSize;
    std::unique_ptr<char[]> storage;
};

}

//=============================================================================
namespace pointer_stack {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    template <typename Functor>
    function (Functor f)
        : invokePtr  (reinterpret_cast<invokePtr_t>  (invoke<Functor>)),
          createPtr  (reinterpret_cast<createPtr_t>  (create<Functor>)),
          destroyPtr (reinterpret_cast<destroyPtr_t> (destroy<Functor>))
    {
        static_assert (sizeof (Functor) <= sizeof (stack), "Too big!");
        createPtr (std::addressof (stack), std::addressof (f));
    }

    function (const function& other)
    {
        if (other.invokePtr != nullptr)
        {
            invokePtr  = other.invokePtr;
            createPtr  = other.createPtr;
            destroyPtr = other.destroyPtr;

            createPtr (std::addressof (stack), std::addressof (other.stack));
        }
    }

    function& operator= (function const& other)
    {
        if (invokePtr != nullptr)
        {
            destroyPtr (std::addressof (stack));
            invokePtr = nullptr;
        }

        if (other.invokePtr != nullptr)
        {
            invokePtr = other.invokePtr;
            createPtr = other.createPtr;
            destroyPtr = other.destroyPtr;

            createPtr (std::addressof (stack), std::addressof (other.stack));
        }

        return *this;
    }

    function() = default;

    ~function()
    {
        if (invokePtr != nullptr)
            destroyPtr (std::addressof (stack));
    }

    Result operator() (Arguments&&... args) const
    {
        return invokePtr (std::addressof (stack), std::forward<Arguments> (args)...);
    }

private:
    template <typename Functor>
    static Result invoke (Functor* f, Arguments&&... args)
    {
        return (*f)(std::forward<Arguments> (args)...);
    }

    template <typename Functor>
    static void create (Functor* destination, Functor* source)
    {
        new (destination) Functor (*source);
    }

    template <typename Functor>
    static void destroy (Functor* f)
    {
        f->~Functor();
    }

    using invokePtr_t = Result(*)(const void*, Arguments&&...);
    using createPtr_t = void(*)(void*, const void*);
    using destroyPtr_t = void(*)(void*);

    invokePtr_t invokePtr = nullptr;
    createPtr_t createPtr;
    destroyPtr_t destroyPtr;

    typename std::aligned_storage<24>::type stack;
};

}

//=============================================================================
namespace pointer_stack_or_heap {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    template <typename Functor>
    function (Functor f)
        : invokePtr  (reinterpret_cast<invokePtr_t>  (invoke<Functor>)),
          createPtr  (reinterpret_cast<createPtr_t>  (create<Functor>)),
          destroyPtr (reinterpret_cast<destroyPtr_t> (destroy<Functor>))
    {
        if (sizeof (Functor) <= sizeof (stack))
        {
            storagePtr = std::addressof (stack);
        }
        else
        {
            heapSize = sizeof (Functor);
            storagePtr = std::malloc (heapSize);
        }

        createPtr (storagePtr, std::addressof (f));
    }

    function (const function& other)
    {
        if (other.storagePtr != nullptr)
        {
            invokePtr  = other.invokePtr;
            createPtr  = other.createPtr;
            destroyPtr = other.destroyPtr;

            if (other.storagePtr == std::addressof (other.stack))
            {
                storagePtr = std::addressof (stack);
            }
            else
            {
                heapSize = other.heapSize;
                storagePtr = std::malloc (heapSize);
            }

            createPtr (storagePtr, other.storagePtr);
        }
    }

    function& operator= (function const& other)
    {
        if (storagePtr != nullptr)
        {
            destroyPtr (storagePtr);

            if (storagePtr != std::addressof (stack))
                std::free (storagePtr);

            storagePtr = nullptr;
        }

        if (other.storagePtr != nullptr)
        {
            invokePtr  = other.invokePtr;
            createPtr  = other.createPtr;
            destroyPtr = other.destroyPtr;

            if (other.storagePtr == std::addressof (other.stack))
            {
                storagePtr = std::addressof (stack);
            }
            else
            {
                heapSize = other.heapSize;
                storagePtr = std::malloc (heapSize);
            }

            createPtr (storagePtr, other.storagePtr);
        }

        return *this;
    }

    function() = default;

    ~function()
    {
        if (storagePtr != nullptr)
        {
            destroyPtr (storagePtr);

            if (storagePtr != std::addressof (stack))
                std::free (storagePtr);
        }
    }

    Result operator() (Arguments... args) const
    {
        return invokePtr (storagePtr, std::forward<Arguments> (args)...);
    }

private:
    template <typename Functor>
    static Result invoke (Functor* f, Arguments&&... args)
    {
        return (*f)(std::forward<Arguments> (args)...);
    }

    template <typename Functor>
    static void create (Functor* destination, Functor* source)
    {
        new (destination) Functor (*source);
    }

    template <typename Functor>
    static void destroy (Functor* f)
    {
        f->~Functor();
    }

    using invokePtr_t = Result(*)(const void*, Arguments&&...);
    using createPtr_t = void(*)(void*, const void*);
    using destroyPtr_t = void(*)(void*);

    invokePtr_t invokePtr;
    createPtr_t createPtr;
    destroyPtr_t destroyPtr;

    typename std::aligned_storage<24>::type stack;
    int heapSize;
    void* storagePtr = nullptr;
};

}

//=============================================================================
namespace non_type_erased {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    template <typename Functor>
    function (Functor f)
        : functionPtr  (f)
    {}

    function() = default;

    Result operator() (Arguments&&... args) const
    {
        return functionPtr (std::forward<Arguments> (args)...);
    }

    Result(*functionPtr)(Arguments...) = nullptr;
};

}

//=============================================================================
namespace polymorphic_stack {

template <typename>
class function;

template <typename Result, typename... Arguments>
class function<Result (Arguments...)>
{
public:
    virtual ~function() {}
    virtual Result operator() (Arguments&&...) const = 0;
};

template <typename, size_t>
class StackFunction;

template <size_t stackSize, typename Result, typename... Arguments>
class StackFunction<Result (Arguments...), stackSize> final : public function<Result (Arguments...)>
{
public:
    template <typename Functor>
    StackFunction (Functor f)
    {
        static_assert (sizeof (FunctorHolder<Functor, Result, Arguments...>) <= sizeof (stack), "Too big!");
        functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack);
        new (functorHolderPtr) FunctorHolder<Functor, Result, Arguments...> (f);
    }

    StackFunction (const StackFunction& other)
    {
        if (other.functorHolderPtr != nullptr)
        {
            functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack);
            other.functorHolderPtr->copyInto (functorHolderPtr);
        }
    }

    StackFunction& operator= (StackFunction const& other)
    {
        if (functorHolderPtr != nullptr)
        {
            functorHolderPtr->~FunctorHolderBase<Result, Arguments...>();
            functorHolderPtr = nullptr;
        }

        if (other.functorHolderPtr != nullptr)
        {
            functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack);
            other.functorHolderPtr->copyInto (functorHolderPtr);
        }

        return *this;
    }

    StackFunction() = default;

    ~StackFunction()
    {
        if (functorHolderPtr != nullptr)
            functorHolderPtr->~FunctorHolderBase<Result, Arguments...>();
    }

    Result operator() (Arguments&&... args) const override
    {
        return (*functorHolderPtr) (std::forward<Arguments> (args)...);
    }

private:
    template <typename ReturnType, typename... Args>
    struct FunctorHolderBase
    {
        virtual ~FunctorHolderBase() {}
        virtual ReturnType operator()(Args&&...) = 0;
        virtual void copyInto (void*) const = 0;
    };

    template <typename Functor, typename ReturnType, typename... Args>
    struct FunctorHolder final : FunctorHolderBase<Result, Arguments...>
    {
        FunctorHolder (Functor func) : f (func) {}

        ReturnType operator()(Args&&... args) override
        {
            return f (std::forward<Arguments> (args)...);
        }

        void copyInto (void* destination) const override
        {
            new (destination) FunctorHolder (f);
        }

        Functor f;
    };

    FunctorHolderBase<Result, Arguments...>* functorHolderPtr = nullptr;
    typename std::aligned_storage<stackSize>::type stack;
};

}

//=============================================================================
int addOne (int x)
{
    return x + 1;
}

template <typename FunctionType>
static int doWork()
{
    std::array<FunctionType, 24> functions;

    for (auto& f : functions)
        f = addOne;

    int sum = 0;
    for (auto& f : functions)
        sum += f (4);

    return sum;
}

template <typename FunctionType>
static void test (benchmark::State& state)
{
    for (auto _ : state)
        benchmark::DoNotOptimize (doWork<FunctionType>());
}
BENCHMARK_TEMPLATE(test, std                       ::function<int(int)>);
BENCHMARK_TEMPLATE(test, inheritance_heap          ::function<int(int)>);
BENCHMARK_TEMPLATE(test, inheritance_stack         ::function<int(int)>);
BENCHMARK_TEMPLATE(test, inheritance_stack_or_heap ::function<int(int)>);
BENCHMARK_TEMPLATE(test, pointer_heap              ::function<int(int)>);
BENCHMARK_TEMPLATE(test, pointer_stack             ::function<int(int)>);
BENCHMARK_TEMPLATE(test, pointer_stack_or_heap     ::function<int(int)>);
BENCHMARK_TEMPLATE(test, non_type_erased           ::function<int(int)>);

// Comment this line out to run on http://quick-bench.com
BENCHMARK_MAIN();

