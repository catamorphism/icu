// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_DATA_MODEL_IMPL_H
#define MESSAGEFORMAT2_DATA_MODEL_IMPL_H

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages using the draft MessageFormat 2.0.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"

U_NAMESPACE_BEGIN namespace message2 {


class MessageFormatDataModelImpl : public UMemory {
private:
    friend class MessageFormatDataModel;

     // The expressions that are being matched on.
     // Null iff this is a `pattern` message.
     const LocalPointer<MessageFormatDataModel::ExpressionList> selectors;

     // The list of `when` clauses (case arms).
     // Null iff this is a `pattern` message.
     const LocalPointer<MessageFormatDataModel::VariantMap> variants;

     // The pattern forming the body of the message.
     // If this is non-null, then `variants` and `selectors` must be null.
     const LocalPointer<MessageFormatDataModel::Pattern> pattern;

     // Bindings for local variables
     const LocalPointer<MessageFormatDataModel::Bindings> bindings;

     // Normalized version of the input string (optional whitespace omitted)
     // Used for testing purposes
     const LocalPointer<UnicodeString> normalizedInput;

     MessageFormatDataModelImpl(const MessageFormatDataModel::Builder& builder, UErrorCode &errorCode);
};

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_DATA_MODEL_IMPL_H

#endif // U_HIDE_DEPRECATED_API
// eof
