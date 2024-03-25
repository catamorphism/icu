// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/uconfig.h"

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_H
#define MESSAGEFORMAT2_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

/**
 * \file
 * \brief C++ API: Formats messages using the draft MessageFormat 2.0.
 */


#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2_formattable.h"
#include "unicode/messageformat2_function_registry.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
#endif

namespace message2 {

    class CachedFormatters;
    class Environment;
    class MessageContext;
    class ResolvedFunctionOptions;
    class ResolvedSelector;
    class StaticErrors;

  /// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalArray<UnicodeString>;
template class U_I18N_API LocalArray<Formattable>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

    // Arguments
    // ----------

    /**
     *
     * The `MessageArguments` class represents the named arguments to a message.
     * It is immutable and movable. It is not copyable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageArguments : public UObject {
    public:
        /**
         * Message arguments constructor, which takes a map and returns a container
         * of arguments that can be passed to a `MessageFormatter`.
         *
         * @param args A reference to a map from strings (argument names) to `message2::Formattable`
         *        objects (argument values). The keys and values of the map are copied into the result.
         * @param status Input/output error code.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageArguments(const std::map<UnicodeString, Formattable>& args, UErrorCode& status) {
            if (U_FAILURE(status)) {
                return;
            }
            argumentNames = LocalArray<UnicodeString>(new UnicodeString[argsLen = (int32_t) args.size()]);
            arguments = LocalArray<Formattable>(new Formattable[argsLen]);
            if (!argumentNames.isValid() || !arguments.isValid()) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            int32_t i = 0;
            for (auto iter = args.begin(); iter != args.end(); ++iter) {
                argumentNames[i] = iter->first;
                arguments[i] = iter->second;
                i++;
            }
        }
        /**
         * Move operator:
         * The source MessageArguments will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageArguments& operator=(MessageArguments&&) noexcept;
        /**
         * Default constructor.
         * Returns an empty arguments mapping.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageArguments() = default;
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~MessageArguments();
    private:
        friend class MessageContext;

        int32_t findArg(const data_model::VariableName&) const;
        bool hasArgument(const data_model::VariableName&) const;
        const Formattable& getArgument(const data_model::VariableName&) const;

        // Avoids using Hashtable so that code constructing a Hashtable
        // doesn't have to appear in this header file
        LocalArray<UnicodeString> argumentNames;
        LocalArray<Formattable> arguments;
        int32_t argsLen = 0;
    }; // class MessageArguments

    /**
     * <p>MessageFormatter is a Technical Preview API implementing MessageFormat 2.0.
     *
     * <p>See <a target="github" href="https://github.com/unicode-org/message-format-wg/blob/main/spec/syntax.md">the
     * description of the syntax with examples and use cases</a> and the corresponding
     * <a target="github" href="https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf">ABNF</a> grammar.</p>
     *
     * The MessageFormatter class is mutable and movable. It is not copyable.
     * (It is mutable because if it has a custom function registry, the registry may include
     * `FormatterFactory` objects implementing custom formatters, which are allowed to contain
     * mutable state.)
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatter : public UObject {
        // Note: This class does not currently inherit from the existing
        // `Format` class.
    public:
        /**
         * Move assignment operator:
         * The source MessageFormatter will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatter& operator=(MessageFormatter&&) noexcept;
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~MessageFormatter();

        /**
         * Formats the message to a string, using the data model that was previously set or parsed,
         * and the given `arguments` object.
         *
         * @param arguments Reference to message arguments
         * @param status    Input/output error code used to indicate syntax errors, data model
         *                  errors, resolution errors, formatting errors, selection errors, as well
         *                  as other errors (such as memory allocation failures). Partial output
         *                  is still provided in the presence of most error types.
         * @return          The string result of formatting the message with the given arguments.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UnicodeString formatToString(const MessageArguments& arguments, UErrorCode &status);

        /**
         * Accesses the locale that this `MessageFormatter` object was created with.
         *
         * @return A reference to the locale.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Locale& getLocale() const { return locale; }

        /**
         * Serializes the data model as a string in MessageFormat 2.0 syntax.
         *
         * @return result    A string representation of the data model.
         *                   The string is a valid MessageFormat 2.0 message.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UnicodeString getPattern() const;

        /**
         * Accesses the data model referred to by this
         * `MessageFormatter` object.
         *
         * @return A reference to the data model.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const MessageFormatDataModel& getDataModel() const;

        /**
         * The mutable Builder class allows each part of the MessageFormatter to be initialized
         * separately; calling its `build()` method yields an immutable MessageFormatter.
         *
         * Not copyable or movable.
         */
        class U_I18N_API Builder : public UObject {
        private:
            friend class MessageFormatter;

            // The pattern to be parsed to generate the formatted message
            UnicodeString pattern;
            bool hasPattern = false;
            bool hasDataModel = false;
            // The data model to be used to generate the formatted message
            // Ignored if hasPattern
            MessageFormatDataModel dataModel;
            Locale locale;
            // Not owned
            const FunctionRegistry* customFunctionRegistry;

        public:
            /**
             * Sets the locale to use for formatting.
             *
             * @param locale The desired locale.
             * @return       A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setLocale(const Locale& locale);
            /**
             * Sets the pattern to be parsed into a data model. (Parsing is
             * delayed until `build()` is called.) If a data model was
             * previously set, the reference to it held by this builder
             * is removed.
             *
             * @param pattern A string in MessageFormat 2.0 syntax.
             * @return       A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setPattern(const UnicodeString& pattern);
            /**
             * Sets a custom function registry.
             *
             * @param functionRegistry Function registry to use. `functionRegistry` is not
             *        adopted, and the caller must ensure its lifetime contains
             *        the lifetime of the `MessageFormatter` object built by this
             *        builder.
             * @return       A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setFunctionRegistry(const FunctionRegistry* functionRegistry);
            /**
             * Sets a data model. If a pattern was previously set, it is removed.
             *
             * @param dataModel Data model to format. Passed by move.
             * @return       A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setDataModel(MessageFormatDataModel&& dataModel);
            /**
             * Constructs a new immutable MessageFormatter using the pattern or data model
             * that was previously set, and the locale (if it was previously set)
             * or default locale (otherwise).
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param parseError Struct to receive information on the position
             *                   of an error within the pattern (not used if
             *                   the data model is set).
             * @param status    Input/output error code.  If the
             *                  pattern cannot be parsed, or if neither the pattern
             *                  nor the data model is set, set to failure code.
             * @return          The new MessageFormatter object
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            MessageFormatter build(UParseError& parseError, UErrorCode& status) const;
            /**
             * Default constructor.
             * Returns a Builder with the default locale and with no
             * data model or pattern set. Either `setPattern()`
             * or `setDataModel()` has to be called before calling `build()`.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder() : locale(Locale::getDefault()), customFunctionRegistry(nullptr) {}
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Builder();
        }; // class MessageFormatter::Builder

        // TODO: Shouldn't be public; only used for testing
        /**
         * Returns a string consisting of the input with optional spaces removed.
         *
         * @return        A normalized string representation of the input
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& getNormalizedPattern() const { return normalizedInput; }

    private:
        friend class Builder;
        friend class MessageContext;

        MessageFormatter(const MessageFormatter::Builder& builder, UParseError &parseError, UErrorCode &status);

        MessageFormatter() = delete; // default constructor not implemented

        // Do not define default assignment operator
        const MessageFormatter &operator=(const MessageFormatter &) = delete;

        ResolvedSelector resolveVariables(const Environment& env, const data_model::Operand&, MessageContext&, UErrorCode &) const;
        ResolvedSelector resolveVariables(const Environment& env, const data_model::Expression&, MessageContext&, UErrorCode &) const;

        // Selection methods

        // Takes a vector of FormattedPlaceholders
        void resolveSelectors(MessageContext&, const Environment& env, UErrorCode&, UVector&) const;
        // Takes a vector of vectors of strings (input) and a vector of PrioritizedVariants (output)
        void filterVariants(const UVector&, UVector&, UErrorCode&) const;
        // Takes a vector of vectors of strings (input) and a vector of PrioritizedVariants (input/output)
        void sortVariants(const UVector&, UVector&, UErrorCode&) const;
        // Takes a vector of strings (input) and a vector of strings (output)
        void matchSelectorKeys(const UVector&, MessageContext&, ResolvedSelector&& rv, UVector&, UErrorCode&) const;
        // Takes a vector of FormattedPlaceholders (input),
        // and a vector of vectors of strings (output)
        void resolvePreferences(MessageContext&, UVector&, UVector&, UErrorCode&) const;

        // Formatting methods
        [[nodiscard]] FormattedPlaceholder formatLiteral(const data_model::Literal&) const;
        void formatPattern(MessageContext&, const Environment&, const data_model::Pattern&, UErrorCode&, UnicodeString&) const;
        // Formats a call to a formatting function
        [[nodiscard]] FormattedPlaceholder evalFormatterCall(const FunctionName& functionName,
                                                       FormattedPlaceholder&& argument,
                                                       FunctionOptions&& options,
                                                       MessageContext& context,
                                                       UErrorCode& status) const;
        // Formats an expression that appears as a selector
        ResolvedSelector formatSelectorExpression(const Environment& env, const data_model::Expression&, MessageContext&, UErrorCode&) const;
        // Formats an expression that appears in a pattern or as the definition of a local variable
        [[nodiscard]] FormattedPlaceholder formatExpression(const Environment&, const data_model::Expression&, MessageContext&, UErrorCode&) const;
        [[nodiscard]] FunctionOptions resolveOptions(const Environment& env, const OptionMap&, MessageContext&, UErrorCode&) const;
        [[nodiscard]] FormattedPlaceholder formatOperand(const Environment&, const data_model::Operand&, MessageContext&, UErrorCode&) const;
        [[nodiscard]] FormattedPlaceholder evalArgument(const data_model::VariableName&, MessageContext&) const;
        void formatSelectors(MessageContext& context, const Environment& env, UErrorCode &status, UnicodeString& result) const;

        // Function registry methods
        const Formatter* maybeCachedFormatter(MessageContext&, const data_model::FunctionName&, UErrorCode& errorCode) const;

        bool hasCustomFunctionRegistry() const {
            return (customFunctionRegistry != nullptr);
        }

        // Precondition: custom function registry exists
        // Note: this is non-const because the values in the FunctionRegistry are mutable
        // (a FormatterFactory can have mutable state)
        const FunctionRegistry& getCustomFunctionRegistry() const;

        bool isCustomFormatter(const FunctionName&) const;
        FormatterFactory* lookupFormatterFactory(MessageContext&, const FunctionName&, UErrorCode& status) const;
        bool isBuiltInSelector(const FunctionName&) const;
        bool isBuiltInFormatter(const FunctionName&) const;
        bool isCustomSelector(const FunctionName&) const;
        const SelectorFactory* lookupSelectorFactory(MessageContext&, const FunctionName&, UErrorCode&) const;
        bool isSelector(const FunctionName& fn) const { return isBuiltInSelector(fn) || isCustomSelector(fn); }
        bool isFormatter(const FunctionName& fn) const { return isBuiltInFormatter(fn) || isCustomFormatter(fn); }
        const Formatter* maybeCachedFormatter(MessageContext&, const FunctionName&, UErrorCode&);

        Selector* getSelector(MessageContext&, const FunctionName&, UErrorCode&) const;
        const Formatter& getFormatter(MessageContext&, const FunctionName&, UErrorCode&) const;

        // Checking for resolution errors
        void checkDeclarations(MessageContext&, Environment*&, UErrorCode&) const;
        void check(MessageContext&, const Environment&, const data_model::Expression&, UErrorCode&) const;
        void check(MessageContext&, const Environment&, const data_model::Operand&, UErrorCode&) const;
        void check(MessageContext&, const Environment&, const OptionMap&, UErrorCode&) const;

        void initErrors(UErrorCode&);
        void clearErrors() const;
        void cleanup() noexcept;

        // The locale this MessageFormatter was created with
        /* const */ Locale locale;

        // Registry for built-in functions
        FunctionRegistry standardFunctionRegistry;
        // Registry for custom functions; may be null if no custom registry supplied
        // Note: this is *not* owned by the MessageFormatter object
        // The reason for this choice is to have a non-destructive MessageFormatter::Builder,
        // while also not requiring the function registry to be deeply-copyable. Making the
        // function registry copyable would impose a requirement on any implementations
        // of the FormatterFactory and SelectorFactory interfaces to implement a custom
        // clone() method, which is necessary to avoid sharing between copies of the
        // function registry (and thus double-frees)
        // Not deeply immutable (the values in the function registry are mutable,
        // as a FormatterFactory can have mutable state
        const FunctionRegistry* customFunctionRegistry;

        // Data model, representing the parsed message
        MessageFormatDataModel dataModel;

        // Normalized version of the input string (optional whitespace removed)
        UnicodeString normalizedInput;

        // Formatter cache
        // Must be a raw pointer to avoid including the internal header file
        // defining CachedFormatters
        // Owned by `this`
        CachedFormatters* cachedFormatters;

        // Errors -- only used while parsing and checking for data model errors; then
        // the MessageContext keeps track of errors
        // Must be a raw pointer to avoid including the internal header file
        // defining StaticErrors
        // Owned by `this`
        StaticErrors* errors;

        // Tracks built-in formatter objects for deletion, as the FunctionRegistry
        // does not own its values
        UVector* standardFormatters;
    }; // class MessageFormatter

} // namespace message2

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_H

#endif // U_HIDE_DEPRECATED_API
// eof
