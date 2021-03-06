/* Copyright (C) 2016-2018 Alibaba Group Holding Limited

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "gtest/gtest.h"
#include "ps-plus/common/data.h"
#include "ps-plus/server/udf.h"
#include "ps-plus/server/slice.h"
#include "ps-plus/common/initializer/constant_initializer.h"
#include "ps-plus/common/hashmap.h"

using ps::server::Udf;
using ps::server::UdfContext;
using ps::server::UdfRegistry;
using ps::server::Variable;
using ps::server::Slices;
using ps::initializer::ConstantInitializer;
using ps::Initializer;
using ps::DataType;
using ps::TensorShape;
using ps::Tensor;
using ps::Data;
using ps::WrapperData;
using ps::HashMap;

TEST(AdagradUpdater, AdagradUpdater) {
  UdfRegistry* udf_registry = UdfRegistry::Get("AdagradUpdater");
  Udf* udf = udf_registry->Build(std::vector<size_t>({0, 1, 2, 3}), std::vector<size_t>({}));
  UdfContext* ctx = new UdfContext;
  Variable* var = new Variable(new Tensor(DataType::kFloat, TensorShape({4, 8}), new ConstantInitializer(5)), nullptr);
  ctx->SetVariable(var);
  Slices slices{.slice_size = 8, .slice_id = std::vector<size_t>({0, 2}), .dim_part = -1, .variable = var, .writable = true};
  ctx->SetData(0, new WrapperData<Slices>(slices), true);
  ctx->SetData(1, new WrapperData<Tensor>(DataType::kFloat, TensorShape({2, 8}), new ConstantInitializer(2)), true);
  ctx->SetData(2, new WrapperData<double>(3), true);
  ctx->SetData(3, new WrapperData<double>(5), true);

  EXPECT_TRUE(udf->Run(ctx).IsOk());
  for (size_t i = 0; i < 8; i++) {
    EXPECT_EQ(3, var->GetData()->Raw<float>()[i]);
    EXPECT_EQ(3, var->GetData()->Raw<float>()[i + 16]);
    EXPECT_EQ(5, var->GetData()->Raw<float>()[i + 8]);
    EXPECT_EQ(5, var->GetData()->Raw<float>()[i + 24]);
  }
  ctx->SetData(1, new WrapperData<Tensor>(DataType::kFloat, TensorShape({2, 8}), new ConstantInitializer(4)), true);  
  EXPECT_TRUE(udf->Run(ctx).IsOk());
  for (size_t i = 0; i < 8; i++) {
    EXPECT_FLOAT_EQ(0.6, var->GetData()->Raw<float>()[i]);
    EXPECT_FLOAT_EQ(0.6, var->GetData()->Raw<float>()[i + 16]);
    EXPECT_EQ(5, var->GetData()->Raw<float>()[i + 8]);
    EXPECT_EQ(5, var->GetData()->Raw<float>()[i + 24]);
  }

  Slices slices2{.slice_size = 8, .slice_id = std::vector<size_t>({(size_t)ps::HashMap::NOT_ADD_ID, 1}), .dim_part = -1, .variable = var, .writable = true};
  ctx->SetData(0, new WrapperData<Slices>(slices2), true);
  EXPECT_TRUE(udf->Run(ctx).IsOk());
  for (size_t i = 0; i < 8; i++) {
    EXPECT_FLOAT_EQ(0.6, var->GetData()->Raw<float>()[i]);
    EXPECT_FLOAT_EQ(0.6, var->GetData()->Raw<float>()[i + 16]);
    EXPECT_FLOAT_EQ(2.381385, var->GetData()->Raw<float>()[i + 8]);
    EXPECT_EQ(5, var->GetData()->Raw<float>()[i + 24]);
  }  
  Slices slices3{.slice_size = 8, .slice_id = std::vector<size_t>({(size_t)ps::HashMap::NOT_ADD_ID, 1}), .dim_part = -1, .variable = var, .writable = false};
  ctx->SetData(0, new WrapperData<Slices>(slices3), true);
  EXPECT_FALSE(udf->Run(ctx).IsOk());
  ctx->SetData(0, new WrapperData<Slices>(slices2), true);
  ctx->SetData(1, new WrapperData<Tensor>(DataType::kDouble, TensorShape({2, 8}), new ConstantInitializer(2)), true);
  EXPECT_FALSE(udf->Run(ctx).IsOk());
  delete var;
  delete ctx;
  delete udf;
}

