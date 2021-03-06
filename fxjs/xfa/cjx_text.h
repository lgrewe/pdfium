// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_TEXT_H_
#define FXJS_XFA_CJX_TEXT_H_

#include "fxjs/xfa/cjx_object.h"
#include "fxjs/xfa/jse_define.h"

class CXFA_Text;

class CJX_Text final : public CJX_Object {
 public:
  explicit CJX_Text(CXFA_Text* node);
  ~CJX_Text() override;

  // CJX_Object:
  bool DynamicTypeIs(TypeTag eType) const override;

  JSE_PROP(defaultValue); /* {default} */
  JSE_PROP(value);

 private:
  using Type__ = CJX_Text;
  using ParentType__ = CJX_Object;

  static const TypeTag static_type__ = TypeTag::Text;
};

#endif  // FXJS_XFA_CJX_TEXT_H_
