// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_arguments.h"
#include "unicode/messageformat2_data_model_names.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

    using namespace data_model;

    // ------------------------------------------------------
    // MessageArguments

    using Arguments = MessageArguments;

    int32_t Arguments::findArg(const VariableName& arg) const {
        U_ASSERT(argsLen == 0 || arguments.isValid());
        for (int32_t i = 0; i < argsLen; i++) {
            if (argumentNames[i] == arg.identifier()) {
                return i;
            }
        }
        return -1;
    }

    bool Arguments::hasArgument(const VariableName& arg) const {
        int32_t i = findArg(arg);
        return i != -1;
    }

    const Formattable& Arguments::getArgument(const VariableName& arg) const {
        int32_t i = findArg(arg);
        return arguments[i];
    }

    MessageArguments::~MessageArguments() {}

    // Message arguments
    // -----------------

    MessageArguments& MessageArguments::operator=(MessageArguments&& other) noexcept {
        U_ASSERT(other.arguments.isValid() || other.argsLen == 0);
        argsLen = other.argsLen;
        if (argsLen != 0) {
            argumentNames.adoptInstead(other.argumentNames.orphan());
            arguments.adoptInstead(other.arguments.orphan());
        }
        return *this;
    }

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
