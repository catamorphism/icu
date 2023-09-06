// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_FORWARD_DECLS_H
#define MESSAGEFORMAT_DATA_MODEL_FORWARD_DECLS_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_utils.h"

U_NAMESPACE_BEGIN

// TODO
// The idea is to declare just enough code to allow for the Windows #ifs at
// the end of this file, and declare all the inner classes out-of-line

namespace message2 {
    class MessageFormatDataModelImpl;
}

// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
template class U_I18N_API LocalPointer<message2::MessageFormatDataModelImpl>;
#endif

// -----------------------------------------------------------------------
// Public MessageFormatDataModel class

/**
 * <p>MessageFormat2 is a Technical Preview API implementing MessageFormat 2.0.
 * Since it is not final, documentation has not yet been added everywhere.
 *
 * The `MessageFormatDataModel` class describes a parsed representation of the text of a message.
 * This representation is public as higher-level APIs for messages will need to know its public
 * interface: for example, to re-instantiate a parsed message with different values for imported
variables.
 *
 * The MessageFormatDataModel API implements <a target="github"
href="https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md">the
 * specification of the abstract syntax (data model representation)</a> for MessageFormat.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
namespace message2 {
    class U_I18N_API MessageFormatDataModel : public UMemory {
/*
  Classes that represent nodes in the data model are nested inside the
  `MessageFormatDataModel` class.

  Classes such as `Expression`, `Pattern` and `VariantMap` are immutable and
  are constructed using the builder pattern.

  Most classes representing nodes have copy constructors. This is because builders
  contain immutable data that must be copied when calling `build()`, since the builder
  could go out of scope before the immutable result of the builder does. Copying is
  also necessary to prevent unexpected mutation if intermediate builders are saved
  and mutated again after calling `build()`.

  The copy constructors perform a deep copy, for example by copying the entire
  list of options for an `Operator` (and copying the entire underlying vector.)
  Some internal fields should be `const`, but are declared as non-`const` to make
  the copy constructor simpler to implement. (These are noted throughout.) In
  other words, those fields are `const` except during the execution of a copy
  constructor.

  On the other hand, intermediate `Builder` methods that return a `Builder&`
  mutate the state of the builder, so in code like:

  Expression::Builder& exprBuilder = Expression::builder()-> setOperand(foo);
  Expression::Builder& exprBuilder2 = exprBuilder.setOperator(bar);

  the call to `setOperator()` would mutate `exprBuilder`, since `exprBuilder`
  and `exprBuilder2` are references to the same object.

  An alternate choice would be to make `build()` destructive, so that copying would
  be unnecessary. Or, both copying and moving variants of `build()` could be
  provided. Copying variants of the intermediate `Builder` methods could be
  provided as well, if this proved useful.
*/
      public:
        class Binding;
        class Expression;
        class FunctionName;
        class Key;
        class Literal;
        class Operand;
        class Operator;
        class Pattern;
        class PatternPart;
        class Reserved;
        class SelectorKeys;
        class VariableName;
        class VariantMap;
        
        using Bindings = ImmutableVector<Binding>;
        using ExpressionList = ImmutableVector<Expression>;
        using KeyList = ImmutableVector<Key>;
        using OptionMap = OrderedMap<Operand>;

    // Public MessageFormatDataModel methods

    /**
     * Accesses the local variable declarations for this data model.
     *
     * @return A reference to a list of bindings for local variables.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const Bindings& getLocalVariables() const;
    /**
     * Determines what type of message this is.
     *
     * @return true if and only if this data model represents a `selectors` message
     *         (if it represents a `match` construct with selectors and variants).
     *
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    UBool hasSelectors() const;
    /**
     * Accesses the selectors.
     * Precondition: hasSelectors()
     *
     * @return A reference to the selector list.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const ExpressionList& getSelectors() const;
    /**
     * Accesses the variants.
     * Precondition: hasSelectors()
     *
     * @return A reference to the variant map.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const VariantMap& getVariants() const;
    /**
     * Accesses the pattern (in a message without selectors).
     * Precondition: !hasSelectors()
     *
     * @return A reference to the pattern.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const Pattern& getPattern() const;

    class U_I18N_API Builder;

    /**
     * Returns a new `MessageFormatDataModels::Builder` object.
     *
     * @param status  Input/output error code.
     * @return        The new Builder object, which is non-null if U_SUCCESS(status).
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
    virtual ~MessageFormatDataModel();

private:
    // TODO: The actual members are split into a separate class
    // so that they can be declared after all the inner MessageFormatDataModel
    // classes are defined
    LocalPointer<MessageFormatDataModelImpl> impl;

    // Do not define default assignment operator
    const MessageFormatDataModel &operator=(const MessageFormatDataModel &) = delete;

    MessageFormatDataModel(const Builder& builder, UErrorCode &status);
    }; // class MessageFormatDataModel
} // namespace message2

// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
template class U_I18N_API LocalPointer<message2::ImmutableVector<message2::MessageFormatDataModel::Key>::Builder>;
#endif

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_DATA_MODEL_FORWARD_DECLS_H

#endif // U_HIDE_DEPRECATED_API
// eof

