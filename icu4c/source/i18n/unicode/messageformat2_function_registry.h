// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_FUNCTION_REGISTRY_H
#define MESSAGEFORMAT2_FUNCTION_REGISTRY_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/format.h"
#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2_formatting_context.h"
#include "unicode/numberformatter.h"
#include "unicode/unistr.h"
#include "unicode/upluralrules.h"

U_NAMESPACE_BEGIN

class Hashtable;

namespace message2 {

class Formatter;
class Selector;

/**
 * Interface that factory classes for creating formatters must implement.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API FormatterFactory : public UObject {
    // TODO: the coding guidelines say that interface classes
    // shouldn't inherit from UObject, but if I change it so these
    // classes don't, and the individual formatter factory classes
    // inherit from public FormatterFactory, public UObject, then
    // memory leaks ensue
public:
    /**
     * Constructs a new formatter object. This method is not const;
     * formatter factories with local state may be defined.
     *
     * @param locale Locale to be used by the formatter.
     * @param status    Input/output error code.
     * @return The new Formatter, which is non-null if U_SUCCESS(status).
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual Formatter* createFormatter(const Locale& locale, UErrorCode& status) = 0;
    virtual ~FormatterFactory();
}; // class FormatterFactory

/**
 * Interface that factory classes for creating selectors must implement.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API SelectorFactory : public UObject {
public:
    /**
     * Constructs a new selector object.
     *
     * @param locale    Locale to be used by the selector.
     * @param status    Input/output error code.
     * @return          The new selector, which is non-null if U_SUCCESS(status).
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual Selector* createSelector(const Locale& locale, UErrorCode& status) const = 0;
    virtual ~SelectorFactory();
}; // class SelectorFactory

/**
 * Defines mappings from names of formatters and selectors to functions implementing them.
 * The required set of formatter and selector functions is defined in the spec. Users can
 * also define custom formatter and selector functions.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API FunctionRegistry : public UObject {
public:
    /**
     * Looks up a formatter factory by the name of the formatter. The result is non-const,
     * since formatter factories may have local state.
     *
     * @param formatterName Name of the desired formatter.
     * @return The new FormatterFactory, or null if no formatter factory has
     *         been registered under `formatterName`.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    FormatterFactory* getFormatter(const data_model::FunctionName& formatterName) const;
    /**
     * Looks up a selector factory by the name of the selector.
     *
     * @param selectorName Name of the desired selector.
     * @return The new SelectorFactory, or null if no selector factory has
     *         been registered under `selectorName`.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const SelectorFactory* getSelector(const data_model::FunctionName& selectorName) const;

    /**
     * The mutable Builder class allows each formatter and selector factory
     * to be initialized separately; calling its `build()` method yields an
     * immutable FunctionRegistry object.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Builder : public UObject {
    private:
        friend class FunctionRegistry;

        Builder(UErrorCode& status);
        LocalPointer<Hashtable> formatters;
        LocalPointer<Hashtable> selectors;
    public:
        /**
         * Registers a formatter factory to a given formatter name. Adopts `formatterFactory`.
         *
         * @param formatterName Name of the formatter being registered.
         * @param formatterFactory A FormatterFactory object to use for creating `formatterName`
         *        formatters.
         * @param status Input/output error code.
         * @return A reference to the builder.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& setFormatter(const data_model::FunctionName& formatterName, FormatterFactory* formatterFactory, UErrorCode& status);
        /**
         * Registers a selector factory to a given selector name. Adopts `selectorFactory`.
         *
         * @param selectorName Name of the selector being registered.
         * @param selectorFactory A SelectorFactory object to use for creating `selectorName`
         *        selectors.
         * @param status Input/output error code.
         * @return A reference to the builder.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& setSelector(const data_model::FunctionName& selectorName, SelectorFactory* selectorFactory, UErrorCode& status);
        /**
         * Creates an immutable `FunctionRegistry` object with the selectors and formatters
         * that were previously registered. The builder cannot be used after this call.
         *
         * @param status  Input/output error code.
         * @return A reference to the new FunctionRegistry, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FunctionRegistry* build(UErrorCode& status);
        /**
         * Destructor.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
         virtual ~Builder();
    }; // class FunctionRegistry::Builder
   /**
     * Returns a new `FunctionRegistry::Builder` object.
     *
     * @param status  Input/output error code.
     * @return A reference to the new Builder, which is non-null if U_SUCCESS(status).
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    static Builder* builder(UErrorCode& status);
    /**
     * Destructor.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual ~FunctionRegistry();

private:
    friend class Builder;
    friend class MessageContext;
    friend class MessageFormatter;

    // Adopts `f` and `s`
    FunctionRegistry(Hashtable* f, Hashtable* s);

    // Debugging; should only be called on a function registry with
    // all the standard functions registered
    void checkFormatter(const char*) const;
    void checkSelector(const char*) const;
    void checkStandard() const;

    bool hasFormatter(const data_model::FunctionName& f) const;
    bool hasSelector(const data_model::FunctionName& s) const;
    const LocalPointer<Hashtable> formatters;
    const LocalPointer<Hashtable> selectors;
 }; // class FunctionRegistry

/**
 * Interface that formatter classes must implement.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API Formatter : public UObject {
public:
    /**
     * Formats the input passed in `context` by setting an output using one of the
     * `FormattingContext` methods or indicating an error.
     *
     * @param context Formatting context; captures the unnamed function argument,
     *        current output, named options, and output. See the `FormattingContext`
     *        documentation for more details.
     * @param status    Input/output error code. Should not be set directly by the
     *        custom formatter, which should use `FormattingContext::setFormattingWarning()`
     *        to signal errors. The custom formatter may pass `status` to other ICU functions
     *        that can signal errors using this mechanism.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual void format(FormattingContext& context, UErrorCode& status) const = 0;
    virtual ~Formatter();
}; // class Formatter

/**
 * Interface that selector classes must implement.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API Selector : public UObject {
public:
    /**
     * Compares the input passed in `context` to an array of keys, and returns an array of matching
     * keys sorted by preference.
     *
     * @param context Formatting context; captures the unnamed function argument and named options.
     *        See the `FormattingContext` documentation for more details.
     * @param keys An array of pointers to strings that are compared to the input (`context.getFormattableInput()`)
     *        in an implementation-specific way.
     * @param numKeys The length of the `keys` array.
     * @param prefs A mutable reference to an array of pointers to strings. `selectKey()` should set the contents
     *        of `prefs` to a subset of `keys`, with the best match placed at the lowest index.
     * @param numMatching A mutable reference that should be set to the length of the `prefs` array.
     * @param status    Input/output error code. Should not be set directly by the
     *        custom selector, which should use `FormattingContext::setSelectorError()`
     *        to signal errors. The custom selector may pass `status` to other ICU functions
     *        that can signal errors using this mechanism.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual void selectKey(FormattingContext& context, UnicodeString** keys/*[]*/, int32_t numKeys, UnicodeString** prefs/*[]*/, int32_t& numMatching, UErrorCode& status) const = 0;
    virtual ~Selector();
}; // class Selector

} // namespace message2

/// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of MessageFormatter.
// (When building DLLs for Windows this is required.)
// (See messageformat2_data_model_forward_decls.h for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointerBase<message2::FunctionRegistry>;
template class U_I18N_API LocalPointer<message2::FunctionRegistry>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FUNCTION_REGISTRY_H

#endif // U_HIDE_DEPRECATED_API
// eof
