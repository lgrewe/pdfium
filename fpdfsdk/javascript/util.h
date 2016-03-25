// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_JAVASCRIPT_UTIL_H_
#define FPDFSDK_JAVASCRIPT_UTIL_H_

#include <string>
#include <vector>

#include "fpdfsdk/javascript/JS_Define.h"

class util : public CJS_EmbedObj {
 public:
  util(CJS_Object* pJSObject);
  ~util() override;

  FX_BOOL printd(IJS_Context* cc,
                 const std::vector<CJS_Value>& params,
                 CJS_Value& vRet,
                 CFX_WideString& sError);
  FX_BOOL printf(IJS_Context* cc,
                 const std::vector<CJS_Value>& params,
                 CJS_Value& vRet,
                 CFX_WideString& sError);
  FX_BOOL printx(IJS_Context* cc,
                 const std::vector<CJS_Value>& params,
                 CJS_Value& vRet,
                 CFX_WideString& sError);
  FX_BOOL scand(IJS_Context* cc,
                const std::vector<CJS_Value>& params,
                CJS_Value& vRet,
                CFX_WideString& sError);
  FX_BOOL byteToChar(IJS_Context* cc,
                     const std::vector<CJS_Value>& params,
                     CJS_Value& vRet,
                     CFX_WideString& sError);

  static void printx(const std::string& cFormat,
                     const std::string& cSource,
                     std::string& cPurpose);
};

class CJS_Util : public CJS_Object {
 public:
  CJS_Util(v8::Local<v8::Object> pObject) : CJS_Object(pObject) {}
  ~CJS_Util() override {}

  DECLARE_JS_CLASS();

  JS_STATIC_METHOD(printd, util);
  JS_STATIC_METHOD(printf, util);
  JS_STATIC_METHOD(printx, util);
  JS_STATIC_METHOD(scand, util);
  JS_STATIC_METHOD(byteToChar, util);
};

#endif  // FPDFSDK_JAVASCRIPT_UTIL_H_
