// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_PICTURE_H_
#define FXJS_XFA_CJX_PICTURE_H_

#include "fxjs/xfa/cjx_node.h"
#include "fxjs/xfa/jse_define.h"

class CXFA_Picture;

class CJX_Picture final : public CJX_Node {
 public:
  explicit CJX_Picture(CXFA_Picture* node);
  ~CJX_Picture() override;

  // CJX_Object:
  bool DynamicTypeIs(TypeTag eType) const override;

  JSE_PROP(defaultValue); /* {default} */
  JSE_PROP(value);

 private:
  using Type__ = CJX_Picture;
  using ParentType__ = CJX_Node;

  static const TypeTag static_type__ = TypeTag::Picture;
};

#endif  // FXJS_XFA_CJX_PICTURE_H_
