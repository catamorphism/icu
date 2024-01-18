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
#include "unicode/messageformat2_formattable.h"
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
      ResolvedFunctionOption(const UnicodeString& n, const Formattable& f) : name(n), value(f) {}
      ResolvedFunctionOption() {}
      ResolvedFunctionOption(ResolvedFunctionOption&&);
      ResolvedFunctionOption& operator=(ResolvedFunctionOption&& other) noexcept {
          name = std::move(other.name);
          value = std::move(other.value);
          return *this;
    }
    virtual ~ResolvedFunctionOption();
}; // class ResolvedFunctionOption

/**
 * <p>MessageFormatter is a Technical Preview API implementing MessageFormat 2.0.
 * Since it is not final, documentation has not yet been added everywhere.
 *
 * The following class represents the input to a custom function; it encapsulates
 * the function's (unnamed) argument and its named options.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API FormattingContext : public UObject {
    public:
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

    virtual ~FormattingContext();
};

// TODO docs
class U_I18N_API FunctionOptions : public UObject {
 public:
    using FunctionOptionsMap = std::map<UnicodeString, message2::Formattable>;
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
    FunctionOptions() { options = nullptr; }
 private:
    friend class MessageFormatter;
    friend class StandardFunctions;

    explicit FunctionOptions(UVector&&, UErrorCode&);

    const ResolvedFunctionOption* getResolvedFunctionOptions(int32_t& len) const;
    UBool getFunctionOption(const UnicodeString&, Formattable&) const;
    int32_t optionsCount() const { return functionOptionsLen; }

    // Named options passed to functions
    // This is not a Hashtable in order to make it possible for code in a public header file
    // to construct a std::map from it, on-the-fly. Otherwise, it would be impossible to put
    // that code in the header because it would have to call internal Hashtable methods.
    ResolvedFunctionOption* options;
    int32_t functionOptionsLen = 0;
};

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FORMATTING_CONTEXT_H

#endif // U_HIDE_DEPRECATED_API
// eof
