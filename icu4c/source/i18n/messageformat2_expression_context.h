// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_EXPRESSION_CONTEXT_H
#define MESSAGEFORMAT2_EXPRESSION_CONTEXT_H

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages using the draft MessageFormat 2.0.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"

U_NAMESPACE_BEGIN

namespace message2 {

    using namespace data_model;

    // The ExpressionContext contains everything needed to format a specific operand
    // or expression.
    class ExpressionContext : public FormattingContext {
    private:

        // The input state tracks whether the formatter has a Formattable
        // or object input; represents an absent operand; or is in an error state.
        enum InputState {
            FALLBACK,
            NO_OPERAND, // Used when the argument is absent, but there are no errors
            INPUT
        };

        // The output state tracks whether (formatted) numeric or string output
        // has been generated.
        enum OutputState {
            NONE,
            NUMBER,
            STRING
        };

        void clearInput();
        void clearOutput();

        bool hasFunctionName() const;
        const FunctionName& getFunctionName();
        void clearFunctionName();
        // Precondition: hasSelector()
        std::unique_ptr<Selector> getSelector(UErrorCode&) const;
        // Precondition: hasFormatter()
        const Formatter& getFormatter(UErrorCode&);

        void adoptFunctionOptions(UVector*, UErrorCode&);
        void clearFunctionOptions();
        bool tryStringAsNumber(const Formattable&, double&) const;
        UBool getInt64Value(const Formattable&, int64_t& result) const;
        UBool getNumericOption(const UnicodeString&, Formattable&) const;

        void doFormattingCall();
        void doSelectorCall(const UnicodeString[], int32_t, UnicodeString[], int32_t&, UErrorCode&);
        void returnFromFunction();

        void enterState(InputState s);
        void enterState(OutputState s);
        void promoteFallbackToOutput();
        void formatInputWithDefaults(const Locale&, UErrorCode&);

        friend class MessageArguments;
        friend class MessageFormatter;

        MessageContext& context;

        InputState inState;
        OutputState outState;

        // Function name that has been set but not yet invoked on an argument
        FunctionName pendingFunctionName;
        bool hasPendingFunctionName = false;

        // Fallback string to use in case of errors
        UnicodeString fallback;

        // Input arises from literals or a message argument
        Formattable input;

        // Output is returned by a formatting function
        UnicodeString stringOutput;
        number::FormattedNumber numberOutput;

        // Named options passed to functions
        // This is not a Hashtable in order to make it possible for code in a public header file
        // to construct a std::map from it, on-the-fly. Otherwise, it would be impossible to put
        // that code in the header because it would have to call internal Hashtable methods.
        LocalArray<ResolvedFunctionOption> functionOptions;
        int32_t functionOptionsLen = 0;

        MessageContext& messageContext() const { return context; }

        // Resets input and output and uses existing fallback
        void setFallback();
        // Sets fallback string
        void setFallbackTo(const FunctionName&);
        void setFallbackTo(const VariableName&);
        void setFallbackTo(const Literal&);
        // Sets the fallback string as input and exits the error state
        void promoteFallbackToInput();

        void setFunctionName(const FunctionName&);

        void setNoOperand();
        void setInput(const Formattable&);
        void setOutput(const UnicodeString&) override;
        void setOutput(number::FormattedNumber&&) override;

        // If there is a function name, clear it and
        // call the function, setting the input and/or output appropriately
        // Precondition: hasFormatter()
        void evalFormatterCall(const FunctionName&, UErrorCode&);
        // If there is a function name, clear it and
        // call the function, setting the input and/or output appropriately
        // Precondition: hasSelector()
        // Calls the pending selector
        // `keys` and `keysOut` are both vectors of strings
        void evalPendingSelectorCall(const UVector&, UVector&, UErrorCode&);

        const ResolvedFunctionOption* getResolvedFunctionOptions(int32_t& len) const override;
        UBool getFunctionOption(const UnicodeString&, Formattable&) const override;
    public:

        ExpressionContext create() const;

        // Precondition: pending function name is set
        bool hasSelector() const;
        // Precondition: pending function name is set
        bool hasFormatter() const;

        bool isFallback() const;

        UBool hasInput() const override;
        const Formattable& getInput() const override;

        UBool hasStringOutput() const override;
        UBool hasNumberOutput() const override;
        bool hasOutput() { return (hasStringOutput() || hasNumberOutput()); }
        // Just gets existing output, doesn't force evaluation
        const UnicodeString& getStringOutput() const override;
        const number::FormattedNumber& getNumberOutput() const override;
        // Forces evaluation
        void formatToString(const Locale&, UErrorCode&) override;

        // Note: this is provided separately from getOptions() so that internal
        // code, which can't call getOptions(), can query the number of options
        int32_t optionsCount() const override;

        void setSelectorError(const UnicodeString&, UErrorCode&) override;
        void setFormattingError(const UnicodeString&, UErrorCode&) override;

        ExpressionContext(MessageContext&);
        ExpressionContext(ExpressionContext&&);
        virtual ~ExpressionContext();
    };

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_EXPRESSION_CONTEXT_H

#endif // U_HIDE_DEPRECATED_API
// eof

