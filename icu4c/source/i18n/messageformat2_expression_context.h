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

U_NAMESPACE_BEGIN namespace message2 {

class FunctionRegistry;

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
    Selector* getSelector(UErrorCode&) const;
    // Precondition: hasFormatter()
    const Formatter* getFormatter(UErrorCode&);

    void initFunctionOptions(UErrorCode&);
    void addFunctionOption(const UnicodeString&, Formattable*, UErrorCode&);
    void clearFunctionOptions();
    Formattable* getOption(const UnicodeString&, Formattable::Type) const;
    bool tryStringAsNumberOption(const UnicodeString&, double&) const;
    Formattable* getNumericOption(const UnicodeString&) const;

    void doFormattingCall();
    void doSelectorCall(const UnicodeString[], int32_t, UnicodeString[], int32_t&, UErrorCode&);
    void returnFromFunction();

    void enterState(InputState s);
    void enterState(OutputState s);
    void promoteFallbackToOutput();
    void formatInputWithDefaults(const Locale&, UErrorCode&);

    ExpressionContext(MessageContext&, UErrorCode&);

    friend class MessageArguments;
    friend class MessageFormatter;

    MessageContext& context;

    InputState inState;
    OutputState outState;

    // Function name that has been set but not yet invoked on an argument
    LocalPointer<FunctionName> pendingFunctionName;

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
    LocalPointer<Hashtable> functionOptions;
    // Named options passed to functions that have type UObject
    // This must be a separate hash table because objects wrapped in
    // a Formattable will be deleted by the destructor of the Formattable,
    // and object values passed as arguments are not owned
    LocalPointer<Hashtable> functionObjectOptions;


    // Creates a new context with the given `MessageContext` as its parent
    static ExpressionContext* create(MessageContext&, UErrorCode&);
    // Creates a new context sharing this's context and parent
    ExpressionContext* create(UErrorCode&);

    const MessageContext& messageContext() const { return context; }

    // Resets input and output and uses existing fallback
    void setFallback();
    // Sets fallback string
    void setFallbackTo(const FunctionName&);
    void setFallbackTo(const VariableName&);
    void setFallbackTo(const MessageFormatDataModel::Literal&);
    // Sets the fallback string as input and exits the error state
    void promoteFallbackToInput();

    void setFunctionName(const FunctionName&, UErrorCode&);
    // Function name must be set; clears it
    void resolveSelector(Selector*);

    void setStringOption(const UnicodeString&, const UnicodeString&, UErrorCode&);
    void setDateOption(const UnicodeString&, UDate, UErrorCode&);
    void setNumericOption(const UnicodeString&, double, UErrorCode&);
    void setObjectOption(const UnicodeString&, const UObject*, UErrorCode&);

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
    void evalPendingSelectorCall(const UVector&, UVector&, UErrorCode&);

    static Formattable* createFormattable(const UnicodeString&, UErrorCode&);
    static Formattable* createFormattable(double, UErrorCode&);
    static Formattable* createFormattable(int64_t, UErrorCode&);
    static Formattable* createFormattableDate(UDate, UErrorCode&);
    static Formattable* createFormattableDecimal(StringPiece, UErrorCode&);
    static Formattable* createFormattable(const UnicodeString*, int32_t, UErrorCode&);
    static Formattable* createFormattable(const UObject*, UErrorCode&);

    public:

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
    int32_t firstOption() const override;
    int32_t optionsCount() const override;
    const Formattable* nextOption(int32_t&, UnicodeString&) const override;

    void setSelectorError(const UnicodeString&, UErrorCode&) override;
    void setFormattingError(const UnicodeString&, UErrorCode&) override;

    virtual ~ExpressionContext();
};

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_EXPRESSION_CONTEXT_H

#endif // U_HIDE_DEPRECATED_API
// eof
