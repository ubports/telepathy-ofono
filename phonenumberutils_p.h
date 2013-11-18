/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Original source code available at: http://androidxref.com/4.0.4/xref/frameworks/base/telephony/java/android/telephony/PhoneNumberUtils.java
 */

#ifndef PHONENUMBERUTILS_H
#define PHONENUMBERUTILS_H

namespace PhoneNumberUtils 
{

/** True if c is ISO-LATIN characters 0-9, *, # , +, WILD, WAIT, PAUSE   */
bool isNonSeparator(char c) 
{
    return (c >= '0' && c <= '9') || c == '*' || c == '#' || c == '+'
            || c == 'N' || c == ';' || c == ',';
}

/** True if c is ISO-LATIN characters 0-9, *, # , +, WILD  */
bool isDialable(char c) 
{
    return (c >= '0' && c <= '9') || c == '*' || c == '#' || c == '+' || c == 'N';
}

/** True if c is ISO-LATIN characters 0-9 */
bool isISODigit (char c) {
    return c >= '0' && c <= '9';
}

/** or -1 if both are negative */
int minPositive (int a, int b) 
{
    if (a >= 0 && b >= 0) {
        return (a < b) ? a : b;
    } else if (a >= 0) { /* && b < 0 */
        return a;
    } else if (b >= 0) { /* && a < 0 */
        return b;
    } else { /* a < 0 && b < 0 */
        return -1;
    }
}

/** index of the last character of the network portion
 *  (eg anything after is a post-dial string)
 */
int indexOfLastNetworkChar(const QString &a) 
{
    int pIndex, wIndex;
    int origLength;
    int trimIndex;

    origLength = a.length();

    pIndex = a.indexOf(',');
    wIndex = a.indexOf(';');

    trimIndex = minPositive(pIndex, wIndex);

    if (trimIndex < 0) {
        return origLength - 1;
    } else {
        return trimIndex - 1;
    }
}

/** all of a up to len must be an international prefix or
 *  separators/non-dialing digits
 */
bool matchIntlPrefix(const QString &a, int len) 
{
    /* '([^0-9*#+pwn]\+[^0-9*#+pwn] | [^0-9*#+pwn]0(0|11)[^0-9*#+pwn] )$' */
    /*        0       1                           2 3 45               */

    int state = 0;
    for (int i = 0 ; i < len ; i++) {
        char c = a.at(i).toLatin1();

        switch (state) {
            case 0:
                if      (c == '+') state = 1;
                else if (c == '0') state = 2;
                else if (isNonSeparator(c)) return false;
            break;

            case 2:
                if      (c == '0') state = 3;
                else if (c == '1') state = 4;
                else if (isNonSeparator(c)) return false;
            break;

            case 4:
                if      (c == '1') state = 5;
                else if (isNonSeparator(c)) return false;
            break;

            default:
                if (isNonSeparator(c)) return false;
            break;

        }
    }

    return state == 1 || state == 3 || state == 5;
}

/** all of 'a' up to len must match non-US trunk prefix ('0') */
bool matchTrunkPrefix(const QString &a, int len) {
    bool found;

    found = false;

    for (int i = 0 ; i < len ; i++) {
        char c = a.at(i).toLatin1();

        if (c == '0' && !found) {
            found = true;
        } else if (isNonSeparator(c)) {
            return false;
        }
    }

    return found;
}

/** all of 'a' up to len must be a (+|00|011)country code)
 *  We're fast and loose with the country code. Any \d{1,3} matches */
bool matchIntlPrefixAndCC(const QString &a, int len) {
    /*  [^0-9*#+pwn]*(\+|0(0|11)\d\d?\d? [^0-9*#+pwn] $ */
    /*      0          1 2 3 45  6 7  8                 */

    int state = 0;
    for (int i = 0 ; i < len ; i++ ) {
        char c = a.at(i).toLatin1();

        switch (state) {
            case 0:
                if      (c == '+') state = 1;
                else if (c == '0') state = 2;
                else if (isNonSeparator(c)) return false;
            break;

            case 2:
                if      (c == '0') state = 3;
                else if (c == '1') state = 4;
                else if (isNonSeparator(c)) return false;
            break;

            case 4:
                if      (c == '1') state = 5;
                else if (isNonSeparator(c)) return false;
            break;

            case 1:
            case 3:
            case 5:
                if      (isISODigit(c)) state = 6;
                else if (isNonSeparator(c)) return false;
            break;

            case 6:
            case 7:
                if      (isISODigit(c)) state++;
                else if (isNonSeparator(c)) return false;
            break;

            default:
                if (isNonSeparator(c)) return false;
        }
    }

    return state == 6 || state == 7 || state == 8;
}


/**
 * Compare phone numbers a and b, return true if they're identical
 * enough for caller ID purposes.
 *
 * - Compares from right to left
 * - requires MIN_MATCH (7) characters to match
 * - handles common trunk prefixes and international prefixes
 *   (basically, everything except the Russian trunk prefix)
 *
 * Note that this method does not return false even when the two phone numbers
 * are not exactly same; rather; we can call this method "similar()", not "equals()".
 *
 * @hide
 */
bool compareLoosely(const QString &a, const QString &b)
{
     int ia, ib;
     int matched;
     int numNonDialableCharsInA = 0;
     int numNonDialableCharsInB = 0;

     if (a.length() == 0 || b.length() == 0) {
         return false;
     }

     if (a == b) {
         return true;
     }

     ia = indexOfLastNetworkChar (a);
     ib = indexOfLastNetworkChar (b);
     matched = 0;

     while (ia >= 0 && ib >=0) {
         char ca, cb;
         bool skipCmp = false;

         ca = a.at(ia).toLatin1();

         if (!isDialable(ca)) {
             ia--;
             skipCmp = true;
             numNonDialableCharsInA++;
         }

         cb = b.at(ib).toLatin1();

         if (!isDialable(cb)) {
             ib--;
             skipCmp = true;
             numNonDialableCharsInB++;
         }

         if (!skipCmp) {
             if (cb != ca && ca != 'N' && cb != 'N') {
                 break;
             }
             ia--; ib--; matched++;
         }
     }

     if (matched < 7) {
         int effectiveALen = a.length() - numNonDialableCharsInA;
         int effectiveBLen = b.length() - numNonDialableCharsInB;


         // if the number of dialable chars in a and b match, but the matched chars < MIN_MATCH,
         // treat them as equal (i.e. 404-04 and 40404)
         if (effectiveALen == effectiveBLen && effectiveALen == matched) {
             return true;
         }

         return false;
     }

     // At least one string has matched completely;
     if (matched >= 7 && (ia < 0 || ib < 0)) {
         return true;
     }

     /*
      * Now, what remains must be one of the following for a
      * match:
      *
      *  - a '+' on one and a '00' or a '011' on the other
      *  - a '0' on one and a (+,00)<country code> on the other
      *     (for this, a '0' and a '00' prefix would have succeeded above)
      */

     if (matchIntlPrefix(a, ia + 1)
         && matchIntlPrefix (b, ib +1)
     ) {
         return true;
     }

     if (matchTrunkPrefix(a, ia + 1)
         && matchIntlPrefixAndCC(b, ib +1)
     ) {
         return true;
     }

     if (matchTrunkPrefix(b, ib + 1)
         && matchIntlPrefixAndCC(a, ia +1)
     ) {
         return true;
     }

     return false;
}

}

#endif
