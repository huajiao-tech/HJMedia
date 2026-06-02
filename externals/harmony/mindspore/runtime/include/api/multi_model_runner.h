/**
 * Copyright 2025-2026 Huawei Technologies Co., Ltd
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
#ifndef MINDSPORE_LITE_INCLUDE_API_MULTI_MODEL_RUNNER_H_
#define MINDSPORE_LITE_INCLUDE_API_MULTI_MODEL_RUNNER_H_
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include "include/api/status.h"
#include "include/api/types.h"
#include "include/api/context.h"
#include "include/api/dual_abi_helper.h"

namespace mindspore {
using ConfigInfos = std::map<std::string, std::map<std::string, std::string>>;
class ModelImpl;
class MS_API ModelExecutor {
 public:
  /// \brief Constructor of ModelExecutor.
  ModelExecutor() = default;
  /// \brief Constructor of ModelExecutor.
  ///
  /// \param[in] models Which is a vector of ModelImplPtr, used to inference in ModelExecutor.
  /// \param[in] executor_input_names Which is a vector of string, name of ModelExecutor's inputs.
  /// \param[in] executor_output_names Which is a vector of string, name of ModelExecutor's outputs.
  /// \param[in] subgraph_input_names Which is a vector of vector of string, name of every model's inputs in
  ///     ModelExecutor.
  /// \param[in] model_output_tensors Which is a vector of MSTensor, output tensor of model.
  ModelExecutor(const std::vector<std::shared_ptr<ModelImpl>> &models,
                const std::vector<std::string> &executor_input_names,
                const std::vector<std::string> &executor_output_names,
                const std::vector<std::vector<std::string>> &subgraph_input_names,
                const std::vector<std::vector<MSTensor>> &model_output_tensors)
      : models_(models),
        executor_input_names_(executor_input_names),
        executor_output_names_(executor_output_names),
        subgraph_input_names_(subgraph_input_names),
        model_output_tensors_(model_output_tensors) {}
  /// \brief Destructor of ModelExecutor.
  ~ModelExecutor() = default;
  /// \brief Inference ModelExecutor API.
  ///
  /// \param[in] inputs A vector where ModelExecutor inputs are arranged in sequence.
  /// \param[out] outputs Which is a pointer to a vector. The ModelExecutor outputs are filled in the container in
  ///     sequence.
  ///
  /// \return Status.
  Status Predict(const std::vector<MSTensor> &inputs, std::vector<MSTensor> *outputs);
  /// \brief Obtains all input tensors of the ModelExecutor.
  ///
  /// \return The vector that includes all input tensors.
  std::vector<MSTensor> GetInputs() const;
  /// \brief Obtains all output tensors of the ModelExecutor.
  ///
  /// \return The vector that includes all output tensors.
  std::vector<MSTensor> GetOutputs() const;

 private:
  std::vector<std::shared_ptr<ModelImpl>> models_;
  std::vector<std::string> executor_input_names_;
  std::vector<std::string> executor_output_names_;
  std::vector<std::vector<std::string>> subgraph_input_names_;
  std::vector<std::vector<MSTensor>> model_output_tensors_;
  bool initialized_ = false;
};

class MS_API MultiModelRunner {
 public:
  /// \brief Constructor of MultiModelRunner.
  MultiModelRunner() = default;
  /// \brief Destructor of ModelExecutor.
  ~MultiModelRunner() = default;
  /// \brief Load and build a multimodelrunner so that it can run on a device.
  ///
  /// \param[in] model_path Define the model path.
  /// \param[in] model_type Define The type of model file. Options: ModelType::kMindIR.
  /// \param[in] model_context Define the context used to store options during execution.
  ///
  /// \return Status. kSuccess: build success, kLiteModelRebuild: build model repeatedly, Other: other types of errors.
  inline Status Build(const std::string &model_path, const ModelType &model_type,
                      const std::shared_ptr<Context> &model_context = nullptr);
  /// \brief Get ModelExecutors in thr multimodelrunner.
  ///
  /// \return Vector of ModelExecutor.
  std::vector<ModelExecutor> GetModelExecutor() const;

  /// \brief Load config file.
  ///
  /// \param[in] config_path config file path.
  ///
  /// \return Status.
  inline Status LoadConfig(const std::string &config_path);

  /// \brief Update config.
  ///
  /// \param[in] section define the config section.
  /// \param[in] config define the config will be updated.
  ///
  /// \return Status.
  inline Status UpdateConfig(const std::string &section, const std::pair<std::string, std::string> &config);

 private:
  Status Build(const std::vector<char> &model_path, const ModelType &model_type,
               const std::shared_ptr<Context> &model_context);
  Status LoadConfig(const std::vector<char> &config_path);
  Status UpdateConfig(const std::vector<char> &section, const std::pair<std::vector<char>, std::vector<char>> &config);
  std::vector<ModelExecutor> executors_;
  std::vector<std::shared_ptr<ModelImpl>> models_;
  std::vector<std::vector<MSTensor>> model_output_tensors_;
  std::string config_file_ = "";
  ConfigInfos config_info_;
};

Status MultiModelRunner::LoadConfig(const std::string &config_path) { return LoadConfig(StringToChar(config_path)); }

Status MultiModelRunner::UpdateConfig(const std::string &section, const std::pair<std::string, std::string> &config) {
  std::pair<std::vector<char>, std::vector<char>> config_pair = {StringToChar(config.first),
                                                                 StringToChar(config.second)};
  return UpdateConfig(StringToChar(section), config_pair);
}

Status MultiModelRunner::Build(const std::string &model_path, const ModelType &model_type,
                               const std::shared_ptr<Context> &model_context) {
  return Build(StringToChar(model_path), model_type, model_context);
}
}  // namespace mindspore
#endif  // MINDSPORE_LITE_INCLUDE_API_MULTI_MODEL_RUNNER_H_
