#ifndef IS_NBLA_FUNCTION_REGISTRY_HPP
#define IS_NBLA_FUNCTION_REGISTRY_HPP

#include "nbla/context.hpp"
#include "nbla/exception.hpp"
#include "nbla/function.hpp"
#include "nbla/preprocessor_magic.hpp"

#include <functional>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace Is
{
    namespace nbla
    {
        using std::string;
		using std::shared_ptr;
		using std::ostringstream;

        /**
         * @brief Print function backends.
         * 
         * @tparam Item 
         */
        template <typename Item>
        string print_function_items(vector<shared_ptr<Item> items)
        {
            ostringstream ss;
            ss << "[";
            for (auto&& item : items)
            {
                ss << item->backend << ", ";
            }
            ss << "]";
            return ss.str();
        }


        /**
         * @brief Container of FunctionDbItem that can be queried by regex.
         * 
         */
        template <typename Item>
        class NBLA_API FunctionDb
        {
            vector<shared_ptr<Item>> items_;

        public:
            typename Item::function_t query(const vector<string>& backend)
            {
                auto iter = items_.end();
                for (auto& be : backend)
                {
                    iter = std::find_if(items_.begin(), items_.end(), 
                                        [be](const shared_ptr<Item>& item) {
                                            return item->backend == be;
                                        });
                    if (iter != items_.end())
                    {
                        break;
                    }
                }

                NBLA_CHECK(iter != items_.end(), error_code::value,
						   "Any of [%s] could not be found in %s",
							string_join(backend, ", ").c_str(),
							print_function_items<Item>(items_).c_str());

                return (*iter)->function;
            }


            /**
             * @brief Adding function item (FunctionDbItem).
             * 
             * @param item 
             */
            void add(shared_ptr<Item> item)
            {
                items_.push_back(item);
            }
        };



        /**
         * @brief Item of FunctionDb that stores backend key and
         *        a creator function (with variadic template args).
         * @tparam Base 
         * @tparam Args 
         */
        template <typename Base, typename... Args>
        struct FunctionDbItem
        {
            // new shared_ptr<Function>(const Context& ctx, Args...)を格納する関数オブジェクト
            using function_t = std::function<shared_ptr<Base>(const Context& ctx, Args...)>;
            
            // 以下の順番入れ替えダメ
            string backend;
            function_t function; // new shared_ptr<Base>(const Context& ctx, Args...)が入る
        };


        /**
         * @brief This is used as a base class of function registry for each function that has
         *        numerically unique operation. See NBLA_REGISTER_FUNCTION.
         * 
         * @tparam Base 
         * @tparam Args 
         */
        template <typename Base, typename... Args>
        class NBLA_API FunctionRegistry
        {
        public:
            using item_t = FunctionDbItem<Base, Args...>;

            /**
             * @brief Create a new function instance.
             * 
             * 
             * @param ctx 
             * @param args 
             * @return shared_ptr<Base> 
             */
            shared_ptr<Base> create(const Context& ctx, Args... args)
            {
                return function_db_.query(ctx.backend)(ctx, args...); // ここで new shared_ptr<Base>();
            }

            void add(shared_ptr<item_t> item)
            {
                function_db_.add(item);
            }

        protected:
            FunctionDb<item_t> function_db_;
        };


#if __cplusplus < 201703L

    // MSVCとCGG間の互換性の無い処理の回避策
    /*!
    *  This ifdef statement aim to support build on MSVC compiler.
    *  The problem was `,##__VA_ARGS__` only works in GCC, but not works in MSVC compiler.
    *  On the other hand, the workaround by NBLA_VA_ARGS for MSVC breaks GCC preprocessor.
    *  I decided to branch very big portion of source code, but I understand this aproach
    *  is very inefficient and not maintainable. If anybody has an idea,
    *  please let me know or PR is welcom.
    */

    // 20/11/28 shinichi inoue
    // 通常、可変長マクロ
    // #define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
    // を作ると、可変部分は1個以上値を渡さなければならない.
    // LOG("FOOBAR\n") // コンパイルエラー
    // しかし、GCCの場合、##__VA_ARGS__を使えば、0個の場合でも受け取れる.
    // #define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__) // OK
    // MSVCの場合, ...が空の場合、コンパイラがprintf(fmt, __VA_ARGS__)の`,`を気を利かせて消してくれる
    // https://tyfkda.github.io/blog/2015/03/04/var-args.html
    // https://stackoverflow.com/questions/5588855/standard-alternative-to-gccs-va-args-trick#11172679
    // https://docs.microsoft.com/ja-jp/cpp/preprocessor/variadic-macros?view=msvc-160

    #ifdef _MSC_VER

        #define NBLA_REGISTER_FUNCTION_HEADER(NAME, ...)                                                \
            FunctionRegistry<Function, NBLA_EXPAND(__VA_ARGS__)>& get_##NAME##Registry();               \
                                                                                                        \
            shared_ptr<Function> create_##NAME(NBLA_ARGDEFS(const Context&, NBLA_EXPAND(__VA_ARGS__))); \


        #define NBLA_REGISTER_FUNCTION_SOURCE(NAME, ...)                                                \
            FunctionRegistry<Function, NBLA_EXPAND(__VA_ARGS__)>& get_##NAME##Registry()                \
            {                                                                                           \
                static FunctionRegistry<Function, NBLA_EXPAND(__VA_ARGS__)> registry;                   \
                return registry;                                                                        \
            }                                                                                           \
                                                                                                        \
            shared_ptr<Function> create_##NAME(NBLA_ARGDEFS(const Context&, NBLA_EXPAND(__VA_ARGS__)))  \
            {                                                                                           \
                init_cpu();                                                                             \
                return get_##NAME##Registry().create(                                                   \
                    NBLA_ARGS(const Context&, NBLA_EXPAND(__VA_ARGS__)));                               \
            }

        /*!
        *  This will be used inside init method.
        */
        #define NBLA_REGISTER_FUNCTION_IMPL(BASE, CLS, BACKEND, ...)                                    \
            {                                                                                           \
                std::function<shared_ptr<Function>(const Context&, NBLA_EXPAND(__VA_ARGS__))> func =    \
                    [](NBLA_ARGDEFS(const Context&, NBLA_EXPAND(__VA_ARGS__))) {                        \
                        return shared_ptr<Function>(                                                    \
                            new CLS(NBLA_ARGS(const Context&, NBLA_EXPAND(__VA_ARGS__))));              \
                    };                                                                                  \
                                                                                                        \
                using item_t = FunctionDbItem<Function, NBLA_EXPAND(__VA_ARGS__)>;                      \
                get_##BASE##Registry().add(shared_ptr<item_t>(new item_t(BACKEND, func)));              \
            }

    #else // GCC

        #define NBLA_REGISTER_FUNCTION_HEADER(NAME, ...)                                                \
            FunctionRegistry<Function, ##__VA_ARGS__>& get_##NAME##Registry();                          \
                                                                                                        \
            shared_ptr<Function> create_##NAME(NBLA_ARGDEFS(const Context&, ##__VA_ARGS__));            


        #define NBLA_REGISTER_FUNCTION_SOURCE(NAME, ...)                                                \ 
            FunctionRegistry<Function, ##__VA_ARGS__> & get_##NAME##Registry()                          \
            {                                                                                           \
                static FunctionRegistry<Function, ##__VA_ARGS__> registry;                              \
                return registry;                                                                        \
            }                                                                                           \
                                                                                                        \
            shared_ptr<Function> create_##NAME(NBLA_ARGDEFS(const Context&, ##__VA_ARGS__))             \
            {                                                                                           \
                init_cpu();                                                                             \
                return get_##NAME##Registry().create(NBLA_ARGS(const Context&, ##__VA_ARGS__));         \
            }

        
        /*!
        *  This will be used inside init method.
        */
        #define NBLA_REGISTER_FUNCTION_IMPL(BASE, CLS, BACKEND, ...)                                    \
            {                                                                                           \
                std::function<shared_ptr<Function>(const Context&, ##__VA_ARGS__)> func =               \
                    [](NBLA_ARGDEFS(const Context&,## __VA_ARGS__)) {                                   \
                        return shared_ptr<Function>(                                                    \
                            new CLS(NBLA_ARGS(const Context&, ##__VA_ARGS__)));                         \
                    };                                                                                  \
                                                                                                        \
                using item_t = FunctionDbItem<Function, ##__VA_ARGS__>;                                 \
                get_##BASE##Registry().add(shared_ptr<item_t>(new item_t(BACKEND, func)));              \
            }

    #endif   

#else
    /*!
    *  __VA_OPT__(,)を使えば、`,`問題は解決する.
    *  c++20 から導入される...
    */
    #define NBLA_REGISTER_FUNCTION_HEADER(NAME, ...)                                                \
        FunctionRegistry<Function __VA_OPT__(,) __VA_ARGS__>& get_##NAME##Registry();               \
        shared_ptr<Function> create_##NAME(NBLA_ARGDEFS(                                            \
                                            const Context& __VA_OPT__(,) NBLA_EXPAND(__VA_ARGS__)));

    #define NBLA_REGISTER_FUNCTION_SOURCE(NAME, ...)                                                \
        FunctionRegistry<Function __VA_OPT__(,) __VA_ARGS__>& get_##NAME##Registry()                \
        {                                                                                           \
            static FunctionRegistry<Function __VA_OPT__(,) __VA_ARGS__> registry;                   \
            return registry;                                                                        \
        }                                                                                           \
                                                                                                    \
        shared_ptr<Function> create_##NAME(NBLA_ARGDEFS(                                            \
                                           const Context& __VA_OPT__(,) NBLA_EXPAND(__VA_ARGS__)))  \
        {                                                                                           \
            init_cpu();                                                                             \
            return get_##NAME##Registry().create(                                                   \
                NBLA_ARGS(const Context& __VA_OPT__(,) NBLA_EXPAND(__VA_ARGS__)));                  \
        }

    /*!
    *  This will be used inside init method.
    */
    #define NBLA_REGISTER_FUNCTION_IMPL(BASE, CLS, BACKEND, ...)                                    \
        {                                                                                           \
            std::function<shared_ptr<Function>(const Context&, __VA_ARGS__)> func =                 \
                [](NBLA_ARGDEFS(const Context&__VA_OPT__(,) NBLA_EXPAND(__VA_ARGS__))) {            \
                    return shared_ptr<Function>(                                                    \
                        new CLS(NBLA_ARGS(const Context& __VA_OPT__(,) NBLA_EXPAND(__VA_ARGS__)))); \
                };                                                                                  \
                                                                                                    \
            using item_t = FunctionDbItem<Function __VA_OPT__(,) __VA_ARGS__>;                      \
            get_##BASE##Registry().add(shared_ptr<item_t>(new item_t{BACKEND, func}));              \
        }

#endif
    }  // namespace nbla
} // namespace Is
#endif