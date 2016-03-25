// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/javascript/util.h"

#include <time.h>

#include <algorithm>
#include <string>
#include <vector>

#include "core/fxcrt/include/fx_ext.h"
#include "fpdfsdk/include/javascript/IJavaScript.h"
#include "fpdfsdk/javascript/JS_Context.h"
#include "fpdfsdk/javascript/JS_Define.h"
#include "fpdfsdk/javascript/JS_EventHandler.h"
#include "fpdfsdk/javascript/JS_Object.h"
#include "fpdfsdk/javascript/JS_Runtime.h"
#include "fpdfsdk/javascript/JS_Value.h"
#include "fpdfsdk/javascript/PublicMethods.h"
#include "fpdfsdk/javascript/resource.h"

#if _FX_OS_ == _FX_ANDROID_
#include <ctype.h>
#endif

BEGIN_JS_STATIC_CONST(CJS_Util)
END_JS_STATIC_CONST()

BEGIN_JS_STATIC_PROP(CJS_Util)
END_JS_STATIC_PROP()

BEGIN_JS_STATIC_METHOD(CJS_Util)
JS_STATIC_METHOD_ENTRY(printd)
JS_STATIC_METHOD_ENTRY(printf)
JS_STATIC_METHOD_ENTRY(printx)
JS_STATIC_METHOD_ENTRY(scand)
JS_STATIC_METHOD_ENTRY(byteToChar)
END_JS_STATIC_METHOD()

IMPLEMENT_JS_CLASS(CJS_Util, util)

#define UTIL_INT 0
#define UTIL_DOUBLE 1
#define UTIL_STRING 2

namespace {

// Map PDF-style directives to equivalent wcsftime directives. Not
// all have direct equivalents, though.
struct TbConvert {
  const FX_WCHAR* lpszJSMark;
  const FX_WCHAR* lpszCppMark;
};

// Map PDF-style directives lacking direct wcsftime directives to
// the value with which they will be replaced.
struct TbConvertAdditional {
  const FX_WCHAR* lpszJSMark;
  int iValue;
};

const TbConvert TbConvertTable[] = {
    {L"mmmm", L"%B"}, {L"mmm", L"%b"}, {L"mm", L"%m"},   {L"dddd", L"%A"},
    {L"ddd", L"%a"},  {L"dd", L"%d"},  {L"yyyy", L"%Y"}, {L"yy", L"%y"},
    {L"HH", L"%H"},   {L"hh", L"%I"},  {L"MM", L"%M"},   {L"ss", L"%S"},
    {L"TT", L"%p"},
#if defined(_WIN32)
    {L"tt", L"%p"},   {L"h", L"%#I"},
#else
    {L"tt", L"%P"},   {L"h", L"%l"},
#endif
};

int ParseDataType(std::wstring* sFormat) {
  bool bPercent = FALSE;
  for (size_t i = 0; i < sFormat->length(); ++i) {
    wchar_t c = (*sFormat)[i];
    if (c == L'%') {
      bPercent = true;
      continue;
    }

    if (bPercent) {
      if (c == L'c' || c == L'C' || c == L'd' || c == L'i' || c == L'o' ||
          c == L'u' || c == L'x' || c == L'X') {
        return UTIL_INT;
      }
      if (c == L'e' || c == L'E' || c == L'f' || c == L'g' || c == L'G') {
        return UTIL_DOUBLE;
      }
      if (c == L's' || c == L'S') {
        // Map s to S since we always deal internally
        // with wchar_t strings.
        (*sFormat)[i] = L'S';
        return UTIL_STRING;
      }
      if (c == L'.' || c == L'+' || c == L'-' || c == L'#' || c == L' ' ||
          FXSYS_iswdigit(c)) {
        continue;
      }
      break;
    }
  }

  return -1;
}

}  // namespace

util::util(CJS_Object* pJSObject) : CJS_EmbedObj(pJSObject) {}

util::~util() {}

FX_BOOL util::printf(IJS_Context* cc,
                     const std::vector<CJS_Value>& params,
                     CJS_Value& vRet,
                     CFX_WideString& sError) {
  int iSize = params.size();
  if (iSize < 1)
    return FALSE;
  std::wstring c_ConvChar(params[0].ToCFXWideString().c_str());
  std::vector<std::wstring> c_strConvers;
  int iOffset = 0;
  int iOffend = 0;
  c_ConvChar.insert(c_ConvChar.begin(), L'S');
  while (iOffset != -1) {
    iOffend = c_ConvChar.find(L"%", iOffset + 1);
    std::wstring strSub;
    if (iOffend == -1)
      strSub = c_ConvChar.substr(iOffset);
    else
      strSub = c_ConvChar.substr(iOffset, iOffend - iOffset);
    c_strConvers.push_back(strSub);
    iOffset = iOffend;
  }

  std::wstring c_strResult;
  std::wstring c_strFormat;
  for (int iIndex = 0; iIndex < (int)c_strConvers.size(); iIndex++) {
    c_strFormat = c_strConvers[iIndex];
    if (iIndex == 0) {
      c_strResult = c_strFormat;
      continue;
    }

    CFX_WideString strSegment;
    if (iIndex >= iSize) {
      c_strResult += c_strFormat;
      continue;
    }

    switch (ParseDataType(&c_strFormat)) {
      case UTIL_INT:
        strSegment.Format(c_strFormat.c_str(), params[iIndex].ToInt());
        break;
      case UTIL_DOUBLE:
        strSegment.Format(c_strFormat.c_str(), params[iIndex].ToDouble());
        break;
      case UTIL_STRING:
        strSegment.Format(c_strFormat.c_str(),
                          params[iIndex].ToCFXWideString().c_str());
        break;
      default:
        strSegment.Format(L"%S", c_strFormat.c_str());
        break;
    }
    c_strResult += strSegment.GetBuffer(strSegment.GetLength() + 1);
  }

  c_strResult.erase(c_strResult.begin());
  vRet = c_strResult.c_str();
  return TRUE;
}

FX_BOOL util::printd(IJS_Context* cc,
                     const std::vector<CJS_Value>& params,
                     CJS_Value& vRet,
                     CFX_WideString& sError) {
  int iSize = params.size();
  if (iSize < 2)
    return FALSE;

  CJS_Runtime* pRuntime = CJS_Runtime::FromContext(cc);
  CJS_Value p1 = params[0];
  CJS_Value p2 = params[1];
  CJS_Date jsDate(pRuntime);
  if (!p2.ConvertToDate(jsDate)) {
    sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSPRINT1);
    return FALSE;
  }

  if (!jsDate.IsValidDate()) {
    sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSPRINT2);
    return FALSE;
  }

  if (p1.GetType() == CJS_Value::VT_number) {
    CFX_WideString swResult;
    switch (p1.ToInt()) {
      case 0:
        swResult.Format(L"D:%04d%02d%02d%02d%02d%02d", jsDate.GetYear(),
                        jsDate.GetMonth() + 1, jsDate.GetDay(),
                        jsDate.GetHours(), jsDate.GetMinutes(),
                        jsDate.GetSeconds());
        break;
      case 1:
        swResult.Format(L"%04d.%02d.%02d %02d:%02d:%02d", jsDate.GetYear(),
                        jsDate.GetMonth() + 1, jsDate.GetDay(),
                        jsDate.GetHours(), jsDate.GetMinutes(),
                        jsDate.GetSeconds());
        break;
      case 2:
        swResult.Format(L"%04d/%02d/%02d %02d:%02d:%02d", jsDate.GetYear(),
                        jsDate.GetMonth() + 1, jsDate.GetDay(),
                        jsDate.GetHours(), jsDate.GetMinutes(),
                        jsDate.GetSeconds());
        break;
      default:
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSVALUEERROR);
        return FALSE;
    }

    vRet = swResult.c_str();
    return TRUE;
  }

  if (p1.GetType() == CJS_Value::VT_string) {
    if (iSize > 2 && params[2].ToBool()) {
      sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_NOTSUPPORT);
      return FALSE;  // currently, it doesn't support XFAPicture.
    }

    // Convert PDF-style format specifiers to wcsftime specifiers. Remove any
    // pre-existing %-directives before inserting our own.
    std::basic_string<wchar_t> cFormat = p1.ToCFXWideString().c_str();
    cFormat.erase(std::remove(cFormat.begin(), cFormat.end(), '%'),
                  cFormat.end());

    for (size_t i = 0; i < FX_ArraySize(TbConvertTable); ++i) {
      int iStart = 0;
      int iEnd;
      while ((iEnd = cFormat.find(TbConvertTable[i].lpszJSMark, iStart)) !=
             -1) {
        cFormat.replace(iEnd, FXSYS_wcslen(TbConvertTable[i].lpszJSMark),
                        TbConvertTable[i].lpszCppMark);
        iStart = iEnd;
      }
    }

    int iYear = jsDate.GetYear();
    int iMonth = jsDate.GetMonth();
    int iDay = jsDate.GetDay();
    int iHour = jsDate.GetHours();
    int iMin = jsDate.GetMinutes();
    int iSec = jsDate.GetSeconds();

    TbConvertAdditional cTableAd[] = {
        {L"m", iMonth + 1}, {L"d", iDay},
        {L"H", iHour},      {L"h", iHour > 12 ? iHour - 12 : iHour},
        {L"M", iMin},       {L"s", iSec},
    };

    for (size_t i = 0; i < FX_ArraySize(cTableAd); ++i) {
      wchar_t tszValue[16];
      CFX_WideString sValue;
      sValue.Format(L"%d", cTableAd[i].iValue);
      memcpy(tszValue, (wchar_t*)sValue.GetBuffer(sValue.GetLength() + 1),
             (sValue.GetLength() + 1) * sizeof(wchar_t));

      int iStart = 0;
      int iEnd;
      while ((iEnd = cFormat.find(cTableAd[i].lpszJSMark, iStart)) != -1) {
        if (iEnd > 0) {
          if (cFormat[iEnd - 1] == L'%') {
            iStart = iEnd + 1;
            continue;
          }
        }
        cFormat.replace(iEnd, FXSYS_wcslen(cTableAd[i].lpszJSMark), tszValue);
        iStart = iEnd;
      }
    }

    struct tm time = {};
    time.tm_year = iYear - 1900;
    time.tm_mon = iMonth;
    time.tm_mday = iDay;
    time.tm_hour = iHour;
    time.tm_min = iMin;
    time.tm_sec = iSec;

    wchar_t buf[64] = {};
    wcsftime(buf, 64, cFormat.c_str(), &time);
    cFormat = buf;
    vRet = cFormat.c_str();
    return TRUE;
  }

  sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSTYPEERROR);
  return FALSE;
}


FX_BOOL util::printx(IJS_Context* cc,
                     const std::vector<CJS_Value>& params,
                     CJS_Value& vRet,
                     CFX_WideString& sError) {
  int iSize = params.size();
  if (iSize < 2)
    return FALSE;
  CFX_WideString sFormat = params[0].ToCFXWideString();
  CFX_WideString sSource = params[1].ToCFXWideString();
  std::string cFormat = CFX_ByteString::FromUnicode(sFormat).c_str();
  std::string cSource = CFX_ByteString::FromUnicode(sSource).c_str();
  std::string cDest;
  printx(cFormat, cSource, cDest);
  vRet = cDest.c_str();
  return TRUE;
}

void util::printx(const std::string& cFormat,
                  const std::string& cSource2,
                  std::string& cPurpose) {
  std::string cSource(cSource2);
  if (!cPurpose.empty())
    // cPurpose.clear();
    cPurpose.erase();
  int itSource = 0;
  int iSize = cSource.size();
  for (int iIndex = 0; iIndex < (int)cFormat.size() && itSource < iSize;
       iIndex++) {
    char letter = cFormat[iIndex];
    switch (letter) {
      case '?':
        cPurpose += cSource[itSource];
        itSource++;
        break;
      case 'X': {
        while (itSource < iSize) {
          if (std::isdigit(cSource[itSource]) ||
              (cSource[itSource] >= 'a' && cSource[itSource] <= 'z') ||
              (cSource[itSource] >= 'A' && cSource[itSource] <= 'Z')) {
            cPurpose += cSource[itSource];
            itSource++;
            break;
          }
          itSource++;
        }
        break;
      } break;
      case 'A': {
        while (itSource < iSize) {
          if ((cSource[itSource] >= 'a' && cSource[itSource] <= 'z') ||
              (cSource[itSource] >= 'A' && cSource[itSource] <= 'Z')) {
            cPurpose += cSource[itSource];
            itSource++;
            break;
          }
          itSource++;
        }
        break;
      } break;
      case '9': {
        while (itSource < iSize) {
          if (std::isdigit(cSource[itSource])) {
            cPurpose += cSource[itSource];
            itSource++;
            break;
          }
          itSource++;
        }
        break;
      }
      case '*': {
        cPurpose.append(cSource, itSource, iSize - itSource);
        itSource = iSize - 1;
        break;
      }
      case '\\':
        break;
      case '>': {
        for (char& c : cSource)
          c = toupper(c);
        break;
      }
      case '<': {
        for (char& c : cSource)
          c = tolower(c);
        break;
      }
      case '=':
        break;
      default:
        cPurpose += letter;
        break;
    }
  }
}

FX_BOOL util::scand(IJS_Context* cc,
                    const std::vector<CJS_Value>& params,
                    CJS_Value& vRet,
                    CFX_WideString& sError) {
  int iSize = params.size();
  if (iSize < 2)
    return FALSE;

  CFX_WideString sFormat = params[0].ToCFXWideString();
  CFX_WideString sDate = params[1].ToCFXWideString();
  double dDate = JS_GetDateTime();
  if (sDate.GetLength() > 0) {
    dDate = CJS_PublicMethods::MakeRegularDate(sDate, sFormat, nullptr);
  }

  if (!JS_PortIsNan(dDate)) {
    vRet = CJS_Date(CJS_Runtime::FromContext(cc), dDate);
  } else {
    vRet.SetNull();
  }

  return TRUE;
}

FX_BOOL util::byteToChar(IJS_Context* cc,
                         const std::vector<CJS_Value>& params,
                         CJS_Value& vRet,
                         CFX_WideString& sError) {
  int iSize = params.size();
  if (iSize == 0)
    return FALSE;
  int nByte = params[0].ToInt();
  unsigned char cByte = (unsigned char)nByte;
  CFX_WideString csValue;
  csValue.Format(L"%c", cByte);
  vRet = csValue.c_str();
  return TRUE;
}
