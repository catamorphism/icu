// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2016, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
#ifndef _TESTMESSAGEFORMAT2
#define _TESTMESSAGEFORMAT2

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/fmtable.h"
#include "unicode/msgfmt2.h"
#include "intltest.h"

/**
 * TestMessageFormat tests MessageFormat
 */
class TestMessageFormat2: public IntlTest {
public:
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL ) override;

    /** 
     * test MessageFormat with various given patterns
     **/
    void testStaticFormat2(void);
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
