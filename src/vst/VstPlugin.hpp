/*
    Ogler - Use OpenGL shaders in REAPER
    Copyright (C) 2023  Francesco Bertolaccini <francesco@bertolaccini.dev>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "interop.hpp"

#include <optional>
#include <sstream>
#include <string.h>
#include <string_view>

namespace vst {
template <typename T> class VstPlugin {
  vst::AEffect effect;
  vst::Rectangle edit_rect;
  std::string chunkdata;

protected:
  static void copy_string(const void *src_p, void *dst_p, size_t src_sz,
                          size_t dst_sz) {
    size_t i;
    auto src = static_cast<const char *>(src_p);
    auto dst = static_cast<char *>(dst_p);
    for (i = 0; i < src_sz && i < dst_sz - 1; ++i) {
      dst[i] = src[i];
    }
    dst[i] = 0;
  }

  vst::HostCallback *const hostcb;

  void host_automate(int index, float value) noexcept {
    hostcb(get_effect(), HostOpcodes::Automate, index, 0, nullptr, value);
  }

  void host_idle() noexcept {
    hostcb(get_effect(), HostOpcodes::Idle, 0, 0, nullptr, 0);
  }

  std::int64_t host_get_version() noexcept {
    return static_cast<std::int64_t>(
        hostcb(get_effect(), HostOpcodes::GetVendorVersion, 0, 0, nullptr, 0));
  }

  std::string_view host_get_vendor() noexcept {
    return static_cast<const char *>(
        hostcb(get_effect(), HostOpcodes::GetVendorString, 0, 0, nullptr, 0));
  }

  std::string_view host_get_product() noexcept {
    return static_cast<const char *>(
        hostcb(get_effect(), HostOpcodes::GetProductString, 0, 0, nullptr, 0));
  }

  std::int64_t host_get_block_size() noexcept {
    return static_cast<std::int64_t>(
        hostcb(get_effect(), HostOpcodes::GetBlockSize, 0, 0, nullptr, 0));
  }

  std::int64_t host_get_sample_rate() noexcept {
    return static_cast<std::int64_t>(
        hostcb(get_effect(), HostOpcodes::GetSampleRate, 0, 0, nullptr, 0));
  }

  virtual bool has_editor() noexcept { return false; }

  void host_begin_edit(int index) noexcept {
    hostcb(get_effect(), HostOpcodes::BeginEdit, index, 0, nullptr, 0);
  }

  void host_end_edit(int index) noexcept {
    hostcb(get_effect(), HostOpcodes::EndEdit, index, 0, nullptr, 0);
  }

  void host_update_display() noexcept {
    hostcb(get_effect(), HostOpcodes::UpdateDisplay, 0, 0, nullptr, 0);
  }

  virtual void set_parameter(std::int32_t index, float value) noexcept {}
  virtual float get_parameter(std::int32_t index) noexcept { return 0; }

  virtual void init() noexcept {}
  virtual void suspend() noexcept {}
  virtual void resume() noexcept {}

  virtual void set_block_size(std::int64_t size) noexcept {}
  virtual void set_sample_rate(float rate) noexcept {}

  virtual std::string_view get_effect_name() noexcept = 0;
  virtual std::string_view get_vendor_name() noexcept = 0;
  virtual std::string_view get_product_name() noexcept = 0;
  virtual std::int32_t get_vendor_version() noexcept = 0;

  virtual Supported can_do(std::string_view s) noexcept {
    return Supported::No;
  }

  virtual PluginCategory get_category() noexcept {
    return PluginCategory::Unknown;
  }

  virtual std::int64_t get_tail_size() noexcept { return 0; }

  virtual void process(float **inputs, float **outputs,
                       std::int32_t num_samples) noexcept {}
  virtual void process(double **inputs, double **outputs,
                       std::int32_t num_samples) noexcept {}

  virtual void get_editor_bounds(std::int16_t &top, std::int16_t &left,
                                 std::int16_t &bottom,
                                 std::int16_t &right) noexcept {}
  virtual void open_editor(void *hWnd) noexcept {}
  virtual void close_editor() noexcept {}
  virtual bool is_editor_open() noexcept { return false; }
  virtual void editor_idle() noexcept {}

  virtual void change_preset(int index) noexcept {}
  virtual int get_preset_index() noexcept { return 0; }
  virtual void set_preset_name(std::string_view name) noexcept {}
  virtual std::string_view get_preset_name() noexcept { return ""; }
  virtual std::string_view get_parameter_label(int index) noexcept {
    return "";
  }
  virtual std::string_view get_parameter_text(int index) noexcept { return ""; }
  virtual std::string_view get_parameter_name(int index) noexcept { return ""; }
  virtual bool can_be_automated(int index) noexcept { return false; }
  virtual std::optional<vst::ParameterProperties>
  get_parameter_properties(int index) noexcept {
    return std::nullopt;
  }
  virtual bool string_to_parameter(int index, std::string text) noexcept {
    return false;
  }
  virtual void save_preset_data(std::ostream &s) noexcept {}
  virtual void save_bank_data(std::ostream &s) noexcept {}
  virtual void load_preset_data(std::istream &s) noexcept {}
  virtual void load_bank_data(std::istream &s) noexcept {}

  virtual std::intptr_t vendor_specific(std::int32_t index, std::intptr_t value,
                                        void *ptr, float opt) noexcept {
    return 0;
  }

public:
  AEffect *get_effect() noexcept { return &effect; }

public:
  VstPlugin(vst::HostCallback *hostcb)
      : hostcb(hostcb),
        effect({
            .dispatcher = [](AEffect *effect, PluginOpcode opcode,
                             std::int32_t index, std::intptr_t value, void *ptr,
                             float opt) -> std::intptr_t {
              auto plugin = static_cast<VstPlugin<T> *>(effect->object);
              switch (opcode) {
              case PluginOpcode::Open:
                plugin->init();
                break;
              case PluginOpcode::Close:
                delete plugin;
                break;
              case PluginOpcode::SetProgram:
                plugin->change_preset(value);
                break;
              case PluginOpcode::GetProgram:
                return plugin->get_preset_index();
              case PluginOpcode::SetProgramName:
                plugin->set_preset_name(static_cast<const char *>(ptr));
                break;
              case PluginOpcode::GetProgramName: {
                auto str = plugin->get_preset_name();
                copy_string(str.data(), ptr, str.size(), MaxParamStrLen);
                return 1;
              }
              case PluginOpcode::GetParamLabel: {
                auto str = plugin->get_parameter_label(index);
                copy_string(str.data(), ptr, str.size(), MaxParamStrLen);
                return 1;
              }
              case PluginOpcode::GetParamDisplay: {
                auto str = plugin->get_parameter_text(index);
                copy_string(str.data(), ptr, str.size(), MaxParamStrLen);
                return 1;
              }
              case PluginOpcode::GetParamName: {
                auto str = plugin->get_parameter_name(index);
                copy_string(str.data(), ptr, str.size(), MaxParamStrLen);
                return 1;
              }
              case PluginOpcode::CanBeAutomated:
                return plugin->can_be_automated(index);
              case PluginOpcode::SetSampleRate:
                plugin->set_sample_rate(opt);
                break;
              case PluginOpcode::SetBlockSize:
                plugin->set_block_size(value);
                break;
              case PluginOpcode::MainsChanged:
                if (value) {
                  plugin->resume();
                } else {
                  plugin->suspend();
                }
                break;
              case PluginOpcode::GetEffectName: {
                auto str = plugin->get_effect_name();
                copy_string(str.data(), ptr, str.size(), MaxVendorStrLen);
                return 1;
              }
              case PluginOpcode::GetVendorString: {
                auto str = plugin->get_vendor_name();
                copy_string(str.data(), ptr, str.size(), MaxVendorStrLen);
                return 1;
              }
              case PluginOpcode::GetProductString: {
                auto str = plugin->get_product_name();
                copy_string(str.data(), ptr, str.size(), MaxVendorStrLen);
                return 1;
              }
              case PluginOpcode::GetVendorVersion:
                return plugin->get_vendor_version();
              case PluginOpcode::CanDo:
                return static_cast<std::intptr_t>(
                    plugin->can_do(static_cast<const char *>(ptr)));
              case PluginOpcode::GetTailSize: {
                auto tailSize = plugin->get_tail_size();
                return tailSize ? tailSize : 1;
              }
              case PluginOpcode::GetVstVersion:
                return 2400;
              case PluginOpcode::GetPlugCategory:
                return static_cast<std::intptr_t>(plugin->get_category());
              case PluginOpcode::EditGetRect: {
                plugin->get_editor_bounds(
                    plugin->edit_rect.top, plugin->edit_rect.left,
                    plugin->edit_rect.bottom, plugin->edit_rect.right);
                auto dest = static_cast<vst::Rectangle **>(ptr);
                *dest = &plugin->edit_rect;

                return 1;
              }
              case PluginOpcode::EditOpen:
                plugin->open_editor(ptr);
                break;
              case PluginOpcode::EditClose:
                plugin->close_editor();
                break;
              case PluginOpcode::EditIdle:
                plugin->editor_idle();
                break;
              case PluginOpcode::GetChunk: {
                std::stringstream stream;
                if (index) {
                  plugin->save_preset_data(stream);
                } else {
                  plugin->save_bank_data(stream);
                }

                plugin->chunkdata = stream.str();
                const char **addr = static_cast<const char **>(ptr);
                *addr = plugin->chunkdata.data();
                return plugin->chunkdata.length();
              }
              case PluginOpcode::SetChunk: {
                std::stringstream stream(
                    std::string(static_cast<char *>(ptr), value));
                if (index) {
                  plugin->load_preset_data(stream);
                } else {
                  plugin->load_bank_data(stream);
                }
              } break;
              case PluginOpcode::GetParameterProperties: {
                auto res = plugin->get_parameter_properties(index);
                if (res.has_value()) {
                  *static_cast<ParameterProperties *>(ptr) = *res;
                  return true;
                } else {
                  return false;
                }
              }
              case PluginOpcode::VendorSpecific: {
                return plugin->vendor_specific(index, value, ptr, opt);
              }
              default:
                break;
              }
              return 0;
            },
            .setParameter =
                [](AEffect *effect, std::int32_t index, float parameter) {
                  static_cast<VstPlugin *>(effect->object)
                      ->set_parameter(index, parameter);
                },
            .getParameter = [](AEffect *effect, std::int32_t index) -> float {
              return static_cast<VstPlugin *>(effect->object)
                  ->get_parameter(index);
            },
            .numPrograms = T::num_programs,
            .numParams = T::num_params,
            .numInputs = T::num_inputs,
            .numOutputs = T::num_outputs,
            .flags = T::flags,
            .object = this,
            .uniqueID = T::unique_id,
            .version = T::version,
            .processReplacing = [](AEffect *effect, float **inputs,
                                   float **outputs,
                                   std::int32_t sampleFrames) {},
            .processDoubleReplacing = [](AEffect *effect, double **inputs,
                                         double **outputs,
                                         std::int32_t sampleFrames) {},
        }) {}
  virtual ~VstPlugin() = default;
};
} // namespace vst