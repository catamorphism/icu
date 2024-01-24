// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_CONTEXT_H
#define MESSAGEFORMAT2_CONTEXT_H

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages using the draft MessageFormat 2.0.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/dtptngen.h"
#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2_function_registry.h"
#include "unicode/numberformatter.h"
#include "unicode/utypes.h"

#include "hash.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
#endif

namespace message2 {

    // Helpers

    template<typename T>
    static T* copyArray(const T* source, int32_t& len) { // `len` is an in/out param
        T* dest = new T[len];
        if (dest == nullptr) {
            // Set length to 0 to prevent the
            // array from being accessed
        len = 0;
        } else {
            for (int32_t i = 0; i < len; i++) {
                dest[i] = source[i];
            }
        }
        return dest;
    }

    template<typename T>
    static T* copyVectorToArray(const UVector& source, int32_t& len) {
        len = source.size();
        T* dest = new T[len];
        if (dest == nullptr) {
            // Set length to 0 to prevent the
            // array from being accessed
            len = 0;
        } else {
            for (int32_t i = 0; i < len; i++) {
                dest[i] = *(static_cast<T*>(source.elementAt(i)));
            }
        }
        return dest;
    }

    template<typename T>
    static T* moveVectorToArray(UVector& source, int32_t& len) {
        len = source.size();
        T* dest = new T[len];
        if (dest == nullptr) {
            // Set length to 0 to prevent the
            // array from being accessed
            len = 0;
        } else {
            for (int32_t i = 0; i < len; i++) {
                dest[i] = std::move(*static_cast<T*>(source.elementAt(i)));
            }
        }
        source.removeAllElements();
        return dest;
    }

    inline UVector* createUVector(UErrorCode& status) {
        if (U_FAILURE(status)) {
            return nullptr;
        }
        LocalPointer<UVector> result(new UVector(status));
        if (U_FAILURE(status)) {
            return nullptr;
        }
        result->setDeleter(uprv_deleteUObject);
        return result.orphan();
    }

    template<typename T>
    inline T* create(T&& node, UErrorCode& status) {
        if (U_FAILURE(status)) {
            return nullptr;
        }
        T* result = new T(std::move(node));
        if (result == nullptr) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return result;
    }

    class Formatter;
    class FormatterFactory;
    class SelectorFactory;

    extern void formatDateWithDefaults(const Locale& locale, UDate date, UnicodeString&, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, double toFormat, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int32_t toFormat, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int64_t toFormat, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, StringPiece toFormat, UErrorCode& errorCode);
    extern DateFormat* defaultDateTimeInstance(const Locale&, UErrorCode&);

    using namespace data_model;

    // Intermediate classes used internally in the formatter

    // Closures and environments
    // -------------------------

    class Environment;

    // A closure represents the right-hand side of a variable
    // declaration, along with an environment giving values
    // to its free variables
    class Closure : public UMemory {
    public:
        const Expression& getExpr() const {
            return expr;
        }
        const Environment& getEnv() const {
            return env;
        }
        Closure(const Expression& expression, const Environment& environment) : expr(expression), env(environment) {}
        Closure(Closure&&) = default;

        virtual ~Closure();
    private:

        // An unevaluated expression
        const Expression& expr;
        // The environment mapping names used in this
        // expression to other expressions
        const Environment& env;
    };

    // An environment is represented as a linked chain of
    // non-empty environments, terminating at an empty environment.
    // It's searched using linear search.
    class Environment : public UMemory {
    public:
        virtual bool has(const VariableName&) const = 0;
        virtual const Closure& lookup(const VariableName&) const = 0;
        static Environment* create(UErrorCode&);
        static Environment* create(const VariableName&, Closure&&, Environment*, UErrorCode&);
        virtual ~Environment();
    };

    class NonEmptyEnvironment;
    class EmptyEnvironment : public Environment {
    public:
        EmptyEnvironment() = default;
        virtual ~EmptyEnvironment();

    private:
        friend class Environment;

        bool has(const VariableName&) const override;
        const Closure& lookup(const VariableName&) const override;
        static EmptyEnvironment* create(UErrorCode&);
        static NonEmptyEnvironment* create(const VariableName&, Closure&&, Environment*, UErrorCode&);
    };

    class NonEmptyEnvironment : public Environment {
    private:
        friend class Environment;

        bool has(const VariableName&) const override;
        const Closure& lookup(const VariableName&) const override;
        static NonEmptyEnvironment* create(const VariableName&, Closure&&, const Environment*, UErrorCode&);
        virtual ~NonEmptyEnvironment();
    private:
        friend class Environment;

        NonEmptyEnvironment(const VariableName& v, Closure&& c, Environment* e) : var(v), rhs(std::move(c)), parent(e) {}

        // Maps VariableName onto Closure*
        // Chain of linked environments
        VariableName var;
        Closure rhs;
        const LocalPointer<Environment> parent;
    };

    // PrioritizedVariant

    // For how this class is used, see the references to (integer, variant) tuples
    // in https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection
    class PrioritizedVariant : public UObject {
    public:
        PrioritizedVariant() = default;
        PrioritizedVariant(PrioritizedVariant&&) = default;
        PrioritizedVariant& operator=(PrioritizedVariant&&) noexcept = default;
        UBool operator<(const PrioritizedVariant&) const;
        int32_t priority;
        /* const */ SelectorKeys keys;
        /* const */ Pattern pat;
        PrioritizedVariant(uint32_t p,
                           const SelectorKeys& k,
                           const Pattern& pattern) noexcept : priority(p), keys(k), pat(pattern) {}
        virtual ~PrioritizedVariant();
    }; // class PrioritizedVariant

    // Errors
    // ----------

    class DynamicErrors;
    class StaticErrors;

    // Internal class -- used as a private field in MessageFormatter
    template <typename ErrorType>
    class Error : public UObject {
    public:
        Error(ErrorType ty) : type(ty) {}
        Error(ErrorType ty, const UnicodeString& s) : type(ty), contents(s) {}
        virtual ~Error();
    private:
        friend class DynamicErrors;
        friend class StaticErrors;

        ErrorType type;
        UnicodeString contents;
    }; // class Error

    enum StaticErrorType {
        DuplicateOptionName,
        MissingSelectorAnnotation,
        NonexhaustivePattern,
        SyntaxError,
        VariantKeyMismatchError
    };

    enum DynamicErrorType {
        UnresolvedVariable,
        FormattingError,
        ReservedError,
        SelectorError,
        UnknownFunction,
    };

    using StaticError = Error<StaticErrorType>;
    using DynamicError = Error<DynamicErrorType>;

    // These explicit instantiations have to come before the
    // destructor definitions
    template<>
    Error<StaticErrorType>::~Error();
    template<>
    Error<DynamicErrorType>::~Error();

    class StaticErrors : public UObject {
    private:
        friend class DynamicErrors;

        LocalPointer<UVector> syntaxAndDataModelErrors;
        bool dataModelError = false;
        bool missingSelectorAnnotationError = false;
        bool syntaxError = false;

    public:
        StaticErrors(UErrorCode&);

        void setMissingSelectorAnnotation(UErrorCode&);
        void setDuplicateOptionName(UErrorCode&);
        void addSyntaxError(UErrorCode&);
        bool hasDataModelError() const { return dataModelError; }
        bool hasSyntaxError() const { return syntaxError; }
        bool hasMissingSelectorAnnotationError() const { return missingSelectorAnnotationError; }
        void addError(StaticError&&, UErrorCode&);
        void checkErrors(UErrorCode&);

        const StaticError& first() const;
        StaticErrors(StaticErrors&&) noexcept;
        virtual ~StaticErrors();
    }; // class StaticErrors

    class DynamicErrors : public UObject {
    private:
        const StaticErrors& staticErrors;
        LocalPointer<UVector> resolutionAndFormattingErrors;
        bool formattingError = false;
        bool selectorError = false;
        bool unknownFunctionError = false;
        bool unresolvedVariableError = false;

    public:
        DynamicErrors(const StaticErrors&, UErrorCode&);

        int32_t count() const;
        void setSelectorError(const FunctionName&, UErrorCode&);
        void setReservedError(UErrorCode&);
        void setUnresolvedVariable(const VariableName&, UErrorCode&);
        void setUnknownFunction(const FunctionName&, UErrorCode&);
        void setFormattingError(const FunctionName&, UErrorCode&);
        bool hasDataModelError() const { return staticErrors.hasDataModelError(); }
        bool hasFormattingError() const { return formattingError; }
        bool hasSelectorError() const { return selectorError; }
        bool hasSyntaxError() const { return staticErrors.hasSyntaxError(); }
        bool hasUnknownFunctionError() const { return unknownFunctionError; }
        bool hasMissingSelectorAnnotationError() const { return staticErrors.hasMissingSelectorAnnotationError(); }
        bool hasUnresolvedVariableError() const { return unresolvedVariableError; }
        void addError(DynamicError&&, UErrorCode&);
        void checkErrors(UErrorCode&) const;
        bool hasError() const;
        bool hasStaticError() const;

        const DynamicError& first() const;
        virtual ~DynamicErrors();
    }; // class DynamicErrors

    // Formatter cache
    // --------------

    class MessageArguments;
    class MessageFormatter;

    // Map from function names to Formatters
    class CachedFormatters : public UObject {
    private:
        friend class MessageFormatter;

        // Maps stringified FunctionNames onto Formatter*
        // Adopts its values
        Hashtable cache;
        CachedFormatters();

    public:
        // Returns a pointer because Formatter is an abstract class
        const Formatter* getFormatter(const FunctionName&);
        // Adopts its argument
        void adoptFormatter(const FunctionName&, Formatter*, UErrorCode&);

        CachedFormatters& operator=(const CachedFormatters&) = delete;
        virtual ~CachedFormatters();
    };

    // The context contains all the information needed to process
    // an entire message: arguments, formatter cache, and error list

    class MessageContext : public UMemory {
    public:
        MessageContext(const MessageArguments&, const StaticErrors&, UErrorCode&);

        bool hasGlobal(const VariableName&) const;
        const Formattable& getGlobal(const VariableName&) const;

        // If any errors were set, update `status` accordingly
        void checkErrors(UErrorCode& status) const;
        DynamicErrors& getErrors() { return errors; }

        virtual ~MessageContext();

    private:

        const MessageArguments& arguments; // External message arguments
        // Errors accumulated during parsing/formatting
        DynamicErrors errors;
    }; // class MessageContext

    // Encapsulates a value to be scrutinized by a `match` with its resolved
    // options and the name of the selector
    class ResolvedSelector : public UObject {
    public:
        ResolvedSelector() {}
        ResolvedSelector(const FunctionName& fn,
                         Selector* selector,
                         FunctionOptions&& options,
                         FormattedPlaceholder&& value);
        // Used either for errors, or when selector isn't yet known
        explicit ResolvedSelector(FormattedPlaceholder&& value);
        bool hasSelector() const { return selector.isValid(); }
        const FormattedPlaceholder& argument() const { return value; }
        FormattedPlaceholder&& takeArgument() { return std::move(value); }
        const Selector* getSelector() {
            U_ASSERT(selector.isValid());
            return selector.getAlias();
        }
        FunctionOptions&& takeOptions() {
            return std::move(options);
        }
        const FunctionName& getSelectorName() const { return selectorName; }
        virtual ~ResolvedSelector();
        ResolvedSelector& operator=(ResolvedSelector&&) noexcept;
        ResolvedSelector(ResolvedSelector&&);
    private:
        FunctionName selectorName; // For error reporting
        LocalPointer<Selector> selector;
        FunctionOptions options;
        FormattedPlaceholder value;
    }; // class ResolvedSelector

} // namespace message2

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_CONTEXT_H

#endif // U_HIDE_DEPRECATED_API
// eof
