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
#include <iterator>

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
            OBJECT_INPUT,
            FORMATTABLE_INPUT
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
        const std::shared_ptr<Formatter> getFormatter(UErrorCode&);

        void addFunctionOption(const UnicodeString&, Formattable&&) noexcept;
        void clearFunctionOptions();
        Formattable* getOption(const UnicodeString&, Formattable::Type) const;
        bool tryStringAsNumberOption(const UnicodeString&, double&) const;
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

        /*
          Object and Formattable inputs are stored separately to avoid accidental copying
          of a Formattable containing an object, which would occur if the Formattable
          assignment operator was used. The copy constructor for Formattables assumes that
          an object stored in a Formattable has type Measure. Since MessageFormat allows
          custom functions to take object arguments of any type that inherits from UObject,
          we have to ensure that a Formattable is never copied.
        */
        // Input arises from literals or a message argument
        // Invariant: input.getType != kObject (object Formattables can't be copied)
        Formattable input;
        // (An object input can only originate from a message argument)
        // Invariant: ((isObject && objectInput != nullptr) || (!isObject && objectInput == nullptr)
        const UObject* objectInput;
        const UObject* getObjectInputPointer() const;

        // Output is returned by a formatting function
        UnicodeString stringOutput;
        number::FormattedNumber numberOutput;

        // Named options passed to functions
        FunctionOptionsMap functionOptions;
        // Named options passed to functions that have type UObject
        // This must be a separate map because objects wrapped in
        // a Formattable will be deleted by the destructor of the Formattable,
        // and object values passed as arguments are not owned
        std::map<UnicodeString, const UObject*> functionObjectOptions;

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

        void setStringOption(const UnicodeString&, const UnicodeString&);
        void setDateOption(const UnicodeString&, UDate);
        void setNumericOption(const UnicodeString&, double);
        void setObjectOption(const UnicodeString&, const UObject*) noexcept;

        void setNoOperand();
        void setInput(const UObject*);
        void setInput(const Formattable&);
        void setInput(const UnicodeString&);
        void setObjectInput(UObject*);
        void setOutput(const UnicodeString&) override;
        void setOutput(number::FormattedNumber&&) override;

        // If there is a function name, clear it and
        // call the function, setting the input and/or output appropriately
        // Precondition: hasFormatter()
        void evalFormatterCall(const FunctionName&, UErrorCode&);
        // If there is a function name, clear it and
        // call the function, setting the input and/or output appropriately
        // Precondition: hasSelector()
        void evalPendingSelectorCall(const std::vector<UnicodeString>&, std::vector<UnicodeString>&, UErrorCode&);

    public:

        ExpressionContext create() const;

        // Precondition: pending function name is set
        bool hasSelector() const;
        // Precondition: pending function name is set
        bool hasFormatter() const;

        bool isFallback() const;

        bool hasInput() const { return hasFormattableInput() || hasObjectInput(); }
        UBool hasFormattableInput() const override;
        UBool hasObjectInput() const override;
        const Formattable& getFormattableInput() const override;
        const UObject& getObjectInput() const override;

        UBool hasStringOutput() const override;
        UBool hasNumberOutput() const override;
        bool hasOutput() { return (hasStringOutput() || hasNumberOutput()); }
        // Just gets existing output, doesn't force evaluation
        const UnicodeString& getStringOutput() const override;
        const number::FormattedNumber& getNumberOutput() const override;
        // Forces evaluation
        void formatToString(const Locale&, UErrorCode&) override;

        UBool getStringOption(const UnicodeString&, UnicodeString&) const override;
        UBool getDoubleOption(const UnicodeString&, double&) const override;
        UBool getInt64Option(const UnicodeString&, int64_t&) const override;
        UBool hasObjectOption(const UnicodeString&) const override;
        const UObject& getObjectOption(const UnicodeString&) const override;
        // Function options iterator
        FunctionOptionsMap::const_iterator begin() const override;
        FunctionOptionsMap::const_iterator end() const override;
        int32_t optionsCount() const override;

        void setSelectorError(const UnicodeString&) override;
        void setFormattingError(const UnicodeString&) override;

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
