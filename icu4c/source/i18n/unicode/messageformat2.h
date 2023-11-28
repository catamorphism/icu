// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_H
#define MESSAGEFORMAT2_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages using the draft MessageFormat 2.0.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2_formatting_context.h"
#include "unicode/messageformat2_function_registry.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

namespace message2 {

class CachedFormatters;
class Environment;
class ExpressionContext;
class MessageContext;

// Arguments
// ----------

/**
 *
 * The `MessageArguments` class represents the named arguments to a message.
 * It is immutable and is not copyable or movable.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API MessageArguments : public UObject {
public:
    /**
     * The mutable Builder class allows each message argument to be initialized
     * separately; calling its `build()` method yields an immutable MessageArguments.
     *
     * Builder is not copyable or movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Builder {
    public:
        /**
         * Adds an argument of type `UnicodeString`.
         *
         * @param key The name of the argument.
         * @param value The value of the argument.
         * @param status    Input/output error code.
         * @return          A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& add(const UnicodeString& key, const UnicodeString& value, UErrorCode& status);
        /**
         * Adds an argument of type `double`.
         *
         * @param key The name of the argument.
         * @param value The value of the argument.
         * @param status    Input/output error code.
         * @return          A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addDouble(const UnicodeString& key, double value, UErrorCode& status);
        /**
         * Adds an argument of type `int64_t`.
         *
         * @param key The name of the argument.
         * @param value The value of the argument.
         * @param status    Input/output error code.
         * @return          A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addInt64(const UnicodeString& key, int64_t value, UErrorCode& status);
        /**
         * Adds an argument of type `UDate`.
         *
         * @param key The name of the argument.
         * @param value The value of the argument.
         * @param status    Input/output error code.
         * @return          A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addDate(const UnicodeString& key, UDate value, UErrorCode& status);
        /**
         * Adds an argument of type `StringPiece`, representing a
         * decimal number.
         *
         * @param key The name of the argument.
         * @param value The value of the argument.
         * @param status    Input/output error code.
         * @return          A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addDecimal(const UnicodeString& key, StringPiece value, UErrorCode& status);
        /**
         * Adds an argument of type UnicodeString[]. Adopts `value`.
         *
         * @param key The name of the argument.
         * @param value The value of the argument, interpreted as an array of strings.
         * @param length The length of the array.
         * @param status  Input/output error code.
         * @return        A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& adoptArray(const UnicodeString& key, const UnicodeString* value, int32_t length, UErrorCode& status);
        /**
         * Adds an argument of type UObject*, which must be non-null. Does not
         * adopt this argument.
         *
         * @param key The name of the argument.
         * @param value The value of the argument.
         * @param status  Input/output error code.
         * @return        A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addObject(const UnicodeString& key, const UObject* value, UErrorCode& status);
        /**
         * Creates an immutable `MessageArguments` object with the argument names
         * and values that were added by previous calls. The builder can still be used
         * after this call.
         *
         * @param status  Input/output error code.
         * @return        The new MessageArguments object, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageArguments* build(UErrorCode& status) const;
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~Builder();
    private:
        friend class MessageArguments;
        Builder(UErrorCode&);
        Builder& add(const UnicodeString&, Formattable*, UErrorCode&);
        // For why these aren't LocalPointers, see the comment on
        // MessageFormatter::cachedFormatters
        Hashtable* contents;
        // Keep a separate hash table for objects, which does not
        // own the values
        // This is because a Formattable that wraps an object can't
        // be copied
        // Here, the values are UObjects*
        Hashtable* objectContents;
    }; // class MessageArguments::Builder

    /**
     * Returns a new `MessageArguments::Builder` object.
     *
     * @param status  Input/output error code.
     * @return        The new builder, which is non-null if U_SUCCESS(status).
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    static Builder* builder(UErrorCode& status);
    /**
     * Destructor.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual ~MessageArguments();
private:
    friend class MessageContext;

    bool hasFormattable(const data_model::VariableName&) const;
    bool hasObject(const data_model::VariableName&) const;
    const Formattable& getFormattable(const data_model::VariableName&) const;
    const UObject* getObject(const data_model::VariableName&) const;

    MessageArguments& add(const UnicodeString&, Formattable*, UErrorCode&);
    MessageArguments(const std::map<UnicodeString, Formattable>&, const std::map<UnicodeString, const UObject*>&);

    std::map<UnicodeString, Formattable> contents;
    // Keep a separate map for objects, which does not
    // own the values
    // This is because a Formattable that wraps an object can't
    // be copied
    std::map<UnicodeString, const UObject*> objectContents;
}; // class MessageArguments

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

    std::vector<StaticError> syntaxAndDataModelErrors;
    bool dataModelError = false;
    bool missingSelectorAnnotationError = false;
    bool syntaxError = false;

    public:
    StaticErrors();

  //  int32_t count() const;
    void setMissingSelectorAnnotation(UErrorCode&);
    void addSyntaxError(UErrorCode&);
    bool hasDataModelError() const { return dataModelError; }
    bool hasSyntaxError() const { return syntaxError; }
    bool hasMissingSelectorAnnotationError() const { return missingSelectorAnnotationError; }
    void addError(StaticError, UErrorCode&);
    void checkErrors(UErrorCode&);

    virtual ~StaticErrors();
}; // class StaticErrors

class DynamicErrors : public UObject {
    private:
    const StaticErrors& staticErrors;
    std::vector<DynamicError> resolutionAndFormattingErrors;
    bool formattingError = false;
    bool selectorError = false;
    bool unknownFunctionError = false;
    bool unresolvedVariableError = false;

    public:
    DynamicErrors(const StaticErrors&);

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
    void addError(DynamicError, UErrorCode&);
    void checkErrors(UErrorCode&) const;
    bool hasError() const;

    virtual ~DynamicErrors();
}; // class DynamicErrors

/**
 * <p>MessageFormatter is a Technical Preview API implementing MessageFormat 2.0.
 *
 * <p>See <a target="github" href="https://github.com/unicode-org/message-format-wg/blob/main/spec/syntax.md">the
 * description of the syntax with examples and use cases</a> and the corresponding
 * <a target="github" href="https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf">ABNF</a> grammar.</p>
 *
 * The MessageFormatter class is immutable and is not movable or copyable.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API MessageFormatter : public UObject {
// Note: This class does not currently inherit from the existing
// `Format` class.
public:
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
    UnicodeString formatToString(const MessageArguments& arguments, UErrorCode &status) const;

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

       Builder() : locale(Locale::getDefault()), customFunctionRegistry(nullptr) {}

       // The pattern to be parsed to generate the formatted message
       UnicodeString pattern;
       bool hasPattern = false;
       bool hasDataModel = false;
       // The data model to be used to generate the formatted message
       // Ignored if hasPattern
       MessageFormatDataModel dataModel;
       Locale locale;
       LocalPointer<FunctionRegistry> standardFunctionRegistry;
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
        * @param functionRegistry Function registry to use; this argument is
        *        not adopted, and the caller must ensure its lifetime contains
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
         * @return          The new MessageFormatter object, which is non-null if
         *                  U_SUCCESS(status).
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatter* build(UParseError& parseError, UErrorCode& status) const;
	/**
	 * Destructor.
	 *
	 * @internal ICU 75.0 technology preview
	 * @deprecated This API is for technology preview only.
	 */
	virtual ~Builder();
    }; // class MessageFormatter::Builder

   /**
     * Returns a new `MessageFormatter::Builder` object.
     *
     * @param status  Input/output error code.
     * @return        The new Builder, which is non-null if U_SUCCESS(status).
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    static Builder* builder(UErrorCode& status);

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

     void resolveVariables(const Environment& env, const data_model::Operand&, ExpressionContext&, UErrorCode &) const;
     void resolveVariables(const Environment& env, const data_model::Expression&, ExpressionContext&, UErrorCode &) const;

     // Selection methods
     void resolveSelectors(MessageContext&, const Environment& env, const data_model::ExpressionList&, UErrorCode&, UVector&) const;
     void matchSelectorKeys(const UVector&, ExpressionContext&, UErrorCode&, UVector&) const;
     void resolvePreferences(const UVector&, const data_model::VariantMap&, UErrorCode&, UVector&) const;

     // Formatting methods
     void formatLiteral(const data_model::Literal&, ExpressionContext&) const;
     void formatPattern(MessageContext&, const Environment&, const data_model::Pattern&, UErrorCode&, UnicodeString&) const;
     // Formats an expression that appears as a selector
     void formatSelectorExpression(const Environment& env, const data_model::Expression&, ExpressionContext&, UErrorCode&) const;
     // Formats an expression that appears in a pattern or as the definition of a local variable
     void formatExpression(const Environment&, const data_model::Expression&, ExpressionContext&, UErrorCode&) const;
     void resolveOptions(const Environment& env, const data_model::OptionMap&, ExpressionContext&, UErrorCode&) const;
     void formatOperand(const Environment&, const data_model::Operand&, ExpressionContext&, UErrorCode&) const;
     void evalArgument(const data_model::VariableName&, ExpressionContext&) const;
     void formatSelectors(MessageContext& context, const Environment& env, const data_model::ExpressionList& selectors, const data_model::VariantMap& variants, UErrorCode &status, UnicodeString& result) const;

     // Function registry methods
     const Formatter* maybeCachedFormatter(MessageContext&, const data_model::FunctionName&, UErrorCode& errorCode) const;

     bool hasCustomFunctionRegistry() const {
         return (customFunctionRegistry != nullptr);
     }

     // Precondition: custom function registry exists
     const FunctionRegistry& getCustomFunctionRegistry() const;

     // Checking for resolution errors
     void checkDeclarations(MessageContext&, Environment*&, UErrorCode&) const;
     void check(MessageContext&, const Environment&, const data_model::Expression&, UErrorCode&) const;
     void check(MessageContext&, const Environment&, const data_model::Operand&, UErrorCode&) const;
     void check(MessageContext&, const Environment&, const data_model::OptionMap&, UErrorCode&) const;

     void initErrors(UErrorCode&);
     void clearErrors() const;

     // The locale this MessageFormatter was created with
     const Locale locale;

     // Registry for built-in functions
     LocalPointer<FunctionRegistry> standardFunctionRegistry;
     // Registry for custom functions; may be null if no custom registry supplied
     // Note: this is *not* owned by the MessageFormatter object
     const FunctionRegistry* customFunctionRegistry;

     // Data model, representing the parsed message
     MessageFormatDataModel dataModel;

     // Normalized version of the input string (optional whitespace removed)
     UnicodeString normalizedInput;

     // Formatter cache
     // Note: it would be preferable to use a LocalPointer, but on Windows platforms,
     // a fully-specified template instantiation for LocalPointer<CachedFormatters>
     // would need to be exported. Then, the entire definition of the internal
     // CachedFormatters class would need to be included in a public header
     // file in order for the compiler to generate this template instantiation.
     // (Just forward-declaring CachedFormatters wouldn't work.)
     // To work around this, we just avoid using a LocalPointer<T>
     // as a member of a public class where T is an internal class.
     // (See the comment on the CurrencyPluralInfoWrapper class in
     // number_decimfmtprops.h for a description of a similar problem.)
     CachedFormatters* cachedFormatters;

     // Errors -- only used while parsing and checking for data model errors; then
     // the MessageContext keeps track of errors
     StaticErrors errors;
}; // class MessageFormatter

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_H

#endif // U_HIDE_DEPRECATED_API
// eof
