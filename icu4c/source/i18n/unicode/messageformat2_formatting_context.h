// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_FORMATTING_CONTEXT_H
#define MESSAGEFORMAT2_FORMATTING_CONTEXT_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages using the draft MessageFormat 2.0.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/formattedvalue.h"
#include "unicode/numberformatter.h"
#include "unicode/smpdtfmt.h"

#include <map>

U_NAMESPACE_BEGIN

namespace message2 {

class Selector;
class SelectorFactory;

/**
 *  A `ResolvedFunctionOption` represents the result of evaluating
 * a single named function option. It pairs the given name with the `Formattable`
 * value resulting from evaluating the option's value.
 *
 * `ResolvedFunctionOption` is immutable and is not copyable or movable.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API ResolvedFunctionOption : public UObject {
  private:

    // TODO
    /* const */ UnicodeString name;
    /* const */ Formattable value;

  public:
      const UnicodeString& getName() const { return name; }
      const Formattable& getValue() const { return value; }
      ResolvedFunctionOption(const UnicodeString& n, Formattable&& f) : name(n), value(std::move(f)) {}
      ResolvedFunctionOption(const UnicodeString& n, const Formattable& f) : name(n), value(f) {}
      ResolvedFunctionOption() {}
      ResolvedFunctionOption& operator=(const ResolvedFunctionOption& other) {
          if (this != &other) {
              name = other.name;
              value = other.value;
          }
        return *this;
    }
    virtual ~ResolvedFunctionOption();
}; // class ResolvedFunctionOption

/**
 * <p>MessageFormatter is a Technical Preview API implementing MessageFormat 2.0.
 * Since it is not final, documentation has not yet been added everywhere.
 *
 * The following class represents the input to a custom function; it encapsulates
 * the function's (unnamed) argument and its named options, as well as providing
 * methods for the function to record its output.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API FormattingContext : public UObject {
    public:

    /**
     * Sets the function's output to a string value.
     *
     * @param output The value of the output.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual void setOutput(const UnicodeString& output) = 0;
    /**
     * Sets the function's output to a `number::FormattedNumber` value
     *
     * @param output The value of the output, which is passed by move.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual void setOutput(number::FormattedNumber&& output) = 0;
    /**
     * Indicates that an error occurred during selection, such as an
     * argument with a type that doesn't support selection. Errors are signaled
     * internally to the `FormattingContext` object and propagated at the end of
     * formatting, and are not signaled using the usual `UErrorCode` mechanism
     * (`UErrorCode`s are still used to indicate memory allocation errors and any
     * errors signaled by other ICU functions).
     *
     * @param name Any informative string (usually the name of the selector function).
     * @param status Input/output error code
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual void setSelectorError(const UnicodeString& name, UErrorCode& status) = 0;
    /**
     * Indicates that an error occurred during formatting, such as an argument
     * having a type not supported by this formatter.
     *
     * @param name Any informative string (usually the name of the formatter function).
     * @param status Input/output error code
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual void setFormattingError(const UnicodeString& name, UErrorCode& status) = 0;
    /**
     * Returns true if and only if a `Formattable` argument was supplied to this
     * function. (Object arguments must be checked for using `hasObjectinput()` and
     * are not treated as a `Formattable` wrapping an object.) Each function has
     * at most one argument, so if `hasFormattableInput()` is true,
     * `hasObjectInput()` is false, and vice versa.
     *     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool hasFormattableInput() const = 0;
    /**
     * Accesses the function's argument, assuming it has type  `Formattable`.
     * It is an internal error to call this method if `!hasFormattableInput()`.
     * In particular, if the argument passed in is a UObject*, it is an internal
     * error to call `getFormattableInput()` (`getObjectInput()` must be called instead.)
     *
     * @return A reference to the argument to this function.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual const Formattable& getFormattableInput() const = 0;
    /**
     * Determines the type of input to this function.
     *
     * @return True if and only if a `UObject*` argument was supplied to this
     *         function.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool hasObjectInput() const = 0;
    /**
     * Accesses the function's argument, assuming it has type `UObject`.
     * It is an internal error to call this method if `!hasObjectInput()`.
     *
     * @return A reference to the argument to this function.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual const UObject& getObjectInput() const = 0;
    /**
     * Checks if the argument being passed in already has a formatted
     * result that is a string. This formatted result may be treated as the input
     * to this formatter, or may be overwritten with the result of formatting the
     * original input differently.
     *
     * @return True if and only if formatted string output is present.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool hasStringOutput() const = 0;
    /**
     * Checks if the argument being passed in already has a formatted
     * result that is a number. This formatted result may be treated as the input
     * to this formatter, or may be overwritten with the result of formatting the
     * original input differently.
     *
     * @return True if and only if formatted number output is present.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool hasNumberOutput() const = 0;
    /**
     * Accesses the existing formatted output of this argument as a string.
     * It is an internal error to call this method if `!hasStringOutput()`.
     *
     * @return A reference to the existing formatted string output.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual const UnicodeString& getStringOutput() const = 0;
    /**
     * Accesses the existing formatted output of this argument as a number.
     * It is an internal error to call this method if `!hasNumberOutput()`.
     *
     * @return A reference to the existing formatted number output.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual const number::FormattedNumber& getNumberOutput() const = 0;
    /**
     * Looks up the value of a named string option.
     *
     * @param optionName The name of the option.
     * @param optionValue A mutable reference that is set to the string value of
     *        the option if the named option exists.
     * @return True if and only if a string-typed option named `optionName` exists.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool getStringOption(const UnicodeString& optionName, UnicodeString& optionValue) const = 0;
    /**
     * Looks up the value of a named numeric option of type `double`.
     * The return value is true if and only if there is a `double`-typed option
     * named `optionName`
     *
     * @param optionName The name of the option.
     * @param optionValue A mutable reference that is set to the `double` value of
     *        the option if the named option exists.
     * @return True if and only if a double-typed option named `optionName` exists.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool getDoubleOption(const UnicodeString& optionName, double& optionValue) const = 0;
    /**
     * Looks up the value of a named numeric option of type `int64_t`.
     * The return value is true if and only if there is a `int64_t`-typed option
     * named `optionName`
     *
     * @param optionName The name of the option.
     * @param optionValue A mutable reference that is set to the `double` value of
     *        the option if the named option exists.
     * @return True if and only if a int64-typed option named `optionName` exists.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool getInt64Option(const UnicodeString& optionName, int64_t& optionValue) const = 0;
    /**
     * Checks for a named object option.
     *
     * @param optionName The name of the option.
     * @return True if and only if an object-typed option named `optionName` exists.
     **
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual UBool hasObjectOption(const UnicodeString& optionName) const = 0;
    /**
     * Accesses a named object option.
     * Precondition: the option must exist.
     *
     * @param optionName The name of the option.
     * @return           A reference to the object value of the option.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual const UObject& getObjectOption(const UnicodeString& optionName) const = 0;
    using FunctionOptionsMap = std::map<UnicodeString, Formattable>;
    /**
     * Returns a map of all name-value pairs provided as options to this function,
     * except for any object-valued options (which must be accessed using
     * `getObjectOption()`). The syntactic order of options is not guaranteed to
     * be preserved.
     *
     * @return           A map from strings to `Formattable` values representing
     *                   the results of resolving each option value.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    FunctionOptionsMap getOptions() const {
        int32_t len;
        const ResolvedFunctionOption* resolvedOptions = getResolvedFunctionOptions(len);
        FunctionOptionsMap result;
        for (int32_t i = 0; i < len; i++) {
            const ResolvedFunctionOption& opt = resolvedOptions[i];
            result[opt.getName()] = opt.getValue();
        }
        return result;
    }
    /**
     * Returns the number of function options.
     *
     * @return           The number of options, including object options.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual int32_t optionsCount() const = 0;
    /**
     * Formats the current argument as a string, using defaults. If `hasNumberOutput()` is
     * true, then the string output is set to the result of formatting the number output,
     * and the number output is cleared. If the function's argument is either absent or is
     * a fallback value, the output is the result of formatting the fallback value (which
     * is the default fallback string if the argument is absent). If the function's argument
     * is object-typed, then the argument is treated as a fallback value, since there is
     * no default formatter for objects.
     *
     * @param locale The locale to use for formatting numbers or dates (does not affect
     *        the formatting of a pre-formatted number, if a number output is already present)
     * @param status Input/output error code
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual void formatToString(const Locale& locale, UErrorCode& status) = 0;

    virtual ~FormattingContext();

 private:

    virtual const ResolvedFunctionOption* getResolvedFunctionOptions(int32_t& len) const = 0;
};

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FORMATTING_CONTEXT_H

#endif // U_HIDE_DEPRECATED_API
// eof
