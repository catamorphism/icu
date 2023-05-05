// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2016, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
 * File TMSGFMT.CPP
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   03/24/97    helena      Converted from Java.
 *   07/11/97    helena      Updated to work on AIX.
 *   08/04/97    jfitz       Updated to intltest
 *******************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tmsgfmt2.h"
#include "cmemory.h"
#include "loctest.h"

#include "unicode/errorcode.h"
#include "unicode/normalizer2.h"
#include "unicode/sortkey.h"
#include "unicode/std_string.h"
#include "unicode/stringpiece.h"
#include "unicode/tblcoll.h"
#include "unicode/uiter.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/usetiter.h"
#include "unicode/ustring.h"

#include "unicode/format.h"
#include "unicode/decimfmt.h"
#include "unicode/localpointer.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/messagepattern.h"
#include "unicode/selfmt.h"
#include "unicode/gregocal.h"
#include "unicode/strenum.h"
#include <stdio.h>
#include "uhash.h"

/**
   TODO: For now, this just tests that valid messages are validated by the parser
   and that a certain set is not validated (with correct error diagnostics)
**/
UnicodeString validTestCases[] = {
  /* From Mf2IcuTest.java */
  "{There are {$count} files on {$where}}",
  "{At {$when :datetime timestyle=default} on {$when :datetime datestyle=default}, \
      there was {$what} on planet {$planet :number kind=integer}.}",
  "{The disk \"{$diskName}\" contains {$fileCount} file(s).}",
  "match {$userGender :select}\n\
     when female {{$userName} est all\u00E9e \u00E0 Paris.} \
     when  *     {{$userName} est all\u00E9 \u00E0 Paris.}",
};

#define NUM_VALID_TEST_CASES 4

void
TestMessageFormat2::runIndexedTest(int32_t index, UBool exec,
                                  const char* &name, char* /*par*/) {
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(testMatchGender);
    TESTCASE_AUTO(testStaticFormat2);
    TESTCASE_AUTO(testValidPatterns);
    TESTCASE_AUTO_END;
}

void TestMessageFormat2::testMatchGender() {
  UParseError parseError;
  IcuTestErrorCode errorCode(*this, "testMatchGender");

  uint32_t i = 3;
  MessageFormat2(validTestCases[i], parseError, errorCode);

  if (U_FAILURE(errorCode)) {
    dataerrln("TestMessageFormat2::testValidPatterns #%d - %s", i, u_errorName(errorCode));
    dataerrln("TestMessageFormat2::testValidPatterns #%d - %d %d", i, parseError.line, parseError.offset);
    logln(UnicodeString("TestMessageFormat2::testValidPatterns failed test #") + ((int32_t) i) + UnicodeString(" with error code ")+(int32_t)errorCode);
    return;
  }
}

void TestMessageFormat2::testValidPatterns() {
  UParseError parseError;
  for (uint32_t i = 0; i < NUM_VALID_TEST_CASES; i++) {
    IcuTestErrorCode errorCode(*this, "testValidPatterns");

    MessageFormat2(validTestCases[i], parseError, errorCode);

    if (U_FAILURE(errorCode)) {
        dataerrln("TestMessageFormat2::testValidPatterns #%d - %s", i, u_errorName(errorCode));
        dataerrln("TestMessageFormat2::testValidPatterns #%d - %d %d", i, parseError.line, parseError.offset);
        logln(UnicodeString("TestMessageFormat2::testValidPatterns failed test #") + ((int32_t) i) + UnicodeString(" with error code ")+(int32_t)errorCode);
        return;
    }
  }
}

void TestMessageFormat2::testStaticFormat2()
{
    IcuTestErrorCode errorCode(*this, "testStaticFormat2");
    UHashtable *arguments = uhash_open(uhash_hashChars, uhash_compareChars, NULL, errorCode);
    uhash_setKeyDeleter(arguments, NULL);
    uhash_puti(arguments, (void*) "planet", (int32_t) 7, errorCode);
    uhash_put(arguments, (void*) "what", (void*) "a disturbance in the Force", errorCode);
    UDate date = 8.71068e+011;
    uhash_put(arguments, (void*) "when", &date, errorCode);

//    UnicodeString result;

    /*
      See msgfmt.h -- add a new MessageFormat2 class with a format() method that takes a hash table of arguments
     */

    /*
        UnicodeString pattern =
            "At {$when :datetime timestyle=default} on {$when :datetime datestyle=default}, there was "
            "{$what} on planet {$planet :number kind=integer}.}";
    */

    UnicodeString pattern = "{{$when :datetime timestyle=default}}";

    UParseError parseError;
    MessageFormat2 result = MessageFormat2(pattern, parseError, errorCode);

/*        
    result = MessageFormat::format(
                                   "At {$when :datetime timestyle=default} on {$when :datetime datestyle=default}, there was {$what} on planet {$planet :number kind=integer}.}",
                                   arguments,
                                   result,
                                   errorCode);
*/
    if (U_FAILURE(errorCode)) {
        dataerrln("TestMessageFormat2::testStaticFormat #1 - %s", u_errorName(errorCode));
        dataerrln("TestMessageFormat2::testStaticFormat #1 - %d %d", parseError.line, parseError.offset);
        logln(UnicodeString("TestMessageFormat2::testStaticFormat failed test #1 with error code ")+(int32_t)errorCode);
        return;
    }

    pattern = "{{$when &datetime timestyle=default}}";
    result = MessageFormat2(pattern, parseError, errorCode);
    if (!U_FAILURE(errorCode)) {
        dataerrln("TestMessageFormat2::testStaticFormat #2 - expected test to fail, but it passed");
        logln(UnicodeString("TestMessageFormat2::testStaticFormat failed test #2 with error code ")+(int32_t)errorCode);
        return;
    } else {
      errorCode.reset();
    }

    const UnicodeString expected(
        u"At 12:20:00\u202FPM on Aug 8, 1997, there was a disturbance in the Force on planet 7.");
    /*
        if (result != expected) {
            errln(UnicodeString("TestMessageFormat2::testStaticFormat2 failed on test") +
                UnicodeString("\n     Result: ") + result +
                UnicodeString("\n   Expected: ") + expected );
        }
    */    
}

#endif /* #if !UCONFIG_NO_FORMATTING */
