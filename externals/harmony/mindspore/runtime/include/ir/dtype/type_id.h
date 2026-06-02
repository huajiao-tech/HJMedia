/**
 * This is the C++ adaptation and derivative work of Myia (https://github.com/mila-iqia/myia/).
 *
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_CORE_IR_DTYPE_TYPE_ID_H_
#define MINDSPORE_CORE_IR_DTYPE_TYPE_ID_H_

#include <cstddef>
#if __has_include("include/mindapi/base/type_id.h")
#include "include/mindapi/base/type_id.h"
#include "include/mindapi/base/macros.h"
#else
#include "mindapi/base/type_id.h"
#include "mindapi/base/macros.h"
#endif

namespace mindspore {
namespace abstract {
MS_CORE_API size_t TypeIdSize(TypeId data_type);
}  // namespace abstract
}  // namespace mindspore

#endif  // MINDSPORE_CORE_IR_DTYPE_TYPE_ID_H_
