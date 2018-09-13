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
        return (*functorHolderPtr) (std::forward<Arguments>(args)...);
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
            return f (std::forward<Arguments>(args)...);
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
        functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack[0]);
        new (functorHolderPtr) FunctorHolder<Functor, Result, Arguments...> (f);
    }

    function (const function& other)
    {
        if (other.functorHolderPtr != nullptr)
        {
            functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack[0]);
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
            functorHolderPtr = (FunctorHolderBase<Result, Arguments...>*) std::addressof (stack[0]);
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
        return (*functorHolderPtr) (std::forward<Arguments>(args)...);
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
            return f (std::forward<Arguments>(args)...);
        }

        void copyInto (void* destination) const override
        {
            new (destination) FunctorHolder (f);
        }

        Functor f;
    };

    char stack[24];
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
            functorHolderPtr = (decltype (functorHolderPtr)) std::addressof (stack[0]);
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
            if (other.functorHolderPtr == (decltype (other.functorHolderPtr)) std::addressof (other.stack[0]))
            {
                functorHolderPtr = (decltype (functorHolderPtr)) std::addressof (stack[0]);
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
            if (functorHolderPtr == (decltype (functorHolderPtr)) std::addressof (stack[0]))
                functorHolderPtr->~FunctorHolderBase();
            else
                delete functorHolderPtr;

            functorHolderPtr = nullptr;
        }

        if (other.functorHolderPtr != nullptr)
        {
            if (other.functorHolderPtr == (decltype (other.functorHolderPtr)) std::addressof (other.stack[0]))
            {
                functorHolderPtr = (decltype (functorHolderPtr)) std::addressof (stack[0]);
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
        if (functorHolderPtr == (decltype (functorHolderPtr)) std::addressof (stack[0]))
            functorHolderPtr->~FunctorHolderBase();
        else
            delete functorHolderPtr;
    }

    Result operator() (Arguments&&... args) const
    {
        return (*functorHolderPtr) (std::forward<Arguments>(args)...);
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
            return f (std::forward<Arguments>(args)...);
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

    char stack[24];
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
        createPtr (storage.get(), reinterpret_cast<char*> (&f));
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
        return invokePtr (storage.get(), std::forward<Arguments>(args)...);
    }

private:
    template <typename Functor>
    static Result invoke (Functor* f, Arguments&&... args)
    {
        return (*f)(std::forward<Arguments>(args)...);
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

    using invokePtr_t = Result(*)(char*, Arguments&&...);
    using createPtr_t = void(*)(char*, char*);
    using destroyPtr_t = void(*)(char*);

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
        createPtr (std::addressof (stack[0]), reinterpret_cast<char*> (&f));
    }

    function (const function& other)
    {
        if (other.invokePtr != nullptr)
        {
            invokePtr  = other.invokePtr;
            createPtr  = other.createPtr;
            destroyPtr = other.destroyPtr;

            createPtr (std::addressof (stack[0]), std::addressof (other.stack[0]));
        }
    }

    function& operator= (function const& other)
    {
        if (invokePtr != nullptr)
        {
            destroyPtr (std::addressof (stack[0]));
            invokePtr = nullptr;
        }

        if (other.invokePtr != nullptr)
        {
            invokePtr = other.invokePtr;
            createPtr = other.createPtr;
            destroyPtr = other.destroyPtr;

            createPtr (std::addressof (stack[0]), std::addressof (other.stack[0]));
        }

        return *this;
    }

    function() = default;

    ~function()
    {
        if (invokePtr != nullptr)
            destroyPtr (std::addressof (stack[0]));
    }

    Result operator() (Arguments&&... args) const
    {
        return invokePtr (std::addressof (stack[0]), std::forward<Arguments>(args)...);
    }

private:
    template <typename Functor>
    static Result invoke (Functor* f, Arguments&&... args)
    {
        return (*f)(std::forward<Arguments>(args)...);
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

    using invokePtr_t = Result(*)(const char*, Arguments&&...);
    using createPtr_t = void(*)(char*, const char*);
    using destroyPtr_t = void(*)(char*);

    invokePtr_t invokePtr = nullptr;
    createPtr_t createPtr;
    destroyPtr_t destroyPtr;

    char stack[24];
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
            storagePtr = std::addressof (stack[0]);
        }
        else
        {
            heapSize = sizeof (Functor);
            storagePtr = (char*) std::malloc (heapSize);
        }

        createPtr (storagePtr, reinterpret_cast<char*> (&f));
    }

    function (const function& other)
    {
        if (other.storagePtr != nullptr)
        {
            invokePtr  = other.invokePtr;
            createPtr  = other.createPtr;
            destroyPtr = other.destroyPtr;

            if (other.storagePtr == std::addressof (other.stack[0]))
            {
                storagePtr = std::addressof (stack[0]);
            }
            else
            {
                heapSize = other.heapSize;
                storagePtr = (char*) std::malloc (heapSize);
            }

            createPtr (storagePtr, other.storagePtr);
        }
    }

    function& operator= (function const& other)
    {
        if (storagePtr != nullptr)
        {
            destroyPtr (storagePtr);

            if (storagePtr != std::addressof (stack[0]))
                delete storagePtr;

            storagePtr = nullptr;
        }

        if (other.storagePtr != nullptr)
        {
            invokePtr  = other.invokePtr;
            createPtr  = other.createPtr;
            destroyPtr = other.destroyPtr;

            if (other.storagePtr == std::addressof (other.stack[0]))
            {
                storagePtr = std::addressof (stack[0]);
            }
            else
            {
                heapSize = other.heapSize;
                storagePtr = (char*) std::malloc (heapSize);
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

            if (storagePtr != std::addressof (stack[0]))
                delete storagePtr;
        }
    }

    Result operator() (Arguments... args) const
    {
        return invokePtr (storagePtr, std::forward<Arguments>(args)...);
    }

private:
    template <typename Functor>
    static Result invoke (Functor* f, Arguments&&... args)
    {
        return (*f)(std::forward<Arguments>(args)...);
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

    using invokePtr_t = Result(*)(const char*, Arguments&&...);
    using createPtr_t = void(*)(char*, const char*);
    using destroyPtr_t = void(*)(char*);

    invokePtr_t invokePtr;
    createPtr_t createPtr;
    destroyPtr_t destroyPtr;

    char stack[24];
    int heapSize;
    char* storagePtr = nullptr;
};

}

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
        return functionPtr (std::forward<Arguments>(args)...);
    }

    Result(*functionPtr)(Arguments...) = nullptr;
};

}
//=============================================================================
int addOne (int x)
{
    return x + 1;
}

template <typename FunctionType, int containerSize>
static int doWork()
{
    std::array<FunctionType, containerSize> functions;

    for (int i = 0; i < (int) functions.size(); ++i)
        functions[i] = addOne;

    int sum = 0;
    for (auto& f : functions)
        sum += f (4);

    return sum;
}

template <typename FunctionType>
static void test (benchmark::State& state)
{
    for (auto _ : state)
        benchmark::DoNotOptimize (doWork<FunctionType, 24>());
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
