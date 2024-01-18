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
    // or expression (which is currently just the fallback string and a reference
    // to the parent MessageContext, but could change in the future.)
    class ExpressionContext {
    private:
        friend class MessageFormatter;

        MessageContext& context;

        // Fallback string to use in case of errors
        UnicodeString fallback;

        MessageContext& messageContext() const { return context; }

        // Sets fallback string
        void setFallbackTo(const FunctionName&);
        void setFallbackTo(const VariableName&);
        void setFallbackTo(const Literal&);

    public:

        const UnicodeString& getFallback() const { return fallback; }
        ExpressionContext create() const;
        ExpressionContext(MessageContext&, const UnicodeString&);
        ExpressionContext(ExpressionContext&&);
        virtual ~ExpressionContext();
    }; // class ExpressionContext

    // Encapsulates a value to be scrutinized by a `match` with its resolved
    // options and the name of the selector
    class ResolvedSelector {
    public:
        ResolvedSelector(const FunctionName& fn,
                         Selector* selector,
                         FunctionOptions&& options,
                         FormattedValue&& value);
        // Used either for errors, or when selector isn't yet known
        explicit ResolvedSelector(FormattedValue&& value);
        bool hasSelector() const { return selector.isValid(); }
        const FormattedValue& argument() const { return value; }
        FormattedValue&& takeArgument() { return std::move(value); }
        const Selector* getSelector() {
            U_ASSERT(selector.isValid());
            return selector.getAlias();
        }
        FunctionOptions&& takeOptions() {
            return std::move(options);
        }
        const FunctionName& getSelectorName() const { return selectorName; }
    private:
        FunctionName selectorName; // For error reporting
        LocalPointer<Selector> selector;
        FunctionOptions options;
        FormattedValue value;
    }; // class ResolvedSelector

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_EXPRESSION_CONTEXT_H

#endif // U_HIDE_DEPRECATED_API
// eof

