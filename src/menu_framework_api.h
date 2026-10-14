#pragma once

#include "RE/Skyrim.h"

#include <Windows.h>
#include <cstdint>

namespace MenuFramework
{
	struct Vec2
	{
		float x;
		float y;
	};

	using RenderCallback = void(__stdcall*)();
	using InputCallback = bool(__stdcall*)(RE::InputEvent*);
	using HudCallback = void(__stdcall*)();

	class Api
	{
	public:
		bool Initialize()
		{
			_module = GetModuleHandleW(L"SKSEMenuFramework.dll");
			if (!_module) {
				return false;
			}

			_getApiVersion = GetFunction<GetApiVersionFunction>("GetMenuFrameworkAPIVersion");
			_addSectionItem = GetFunction<AddSectionItemFunction>("AddSectionItem");
			_registerInput = GetFunction<RegisterInputFunction>("RegisterInpoutEvent");
			_registerHud = GetFunction<RegisterHudFunction>("RegisterHudElement");
			_button = GetFunction<ButtonFunction>("igButton");
			_checkbox = GetFunction<CheckboxFunction>("igCheckbox");
			_sliderFloat = GetFunction<SliderFloatFunction>("igSliderFloat");
			_sameLine = GetFunction<SameLineFunction>("igSameLine");
			_textUnformatted = GetFunction<TextUnformattedFunction>("igTextUnformatted");

			return _getApiVersion && _getApiVersion() >= 1 && _addSectionItem && _registerInput && _registerHud &&
				_button && _checkbox && _sliderFloat && _sameLine && _textUnformatted;
		}

		void AddSectionItem(const char* a_path, RenderCallback a_renderer) const
		{
			_addSectionItem(a_path, a_renderer);
		}

		std::int64_t RegisterInput(InputCallback a_callback) const
		{
			return _registerInput(a_callback);
		}

		std::int64_t RegisterHud(HudCallback a_callback) const
		{
			return _registerHud(a_callback);
		}

		bool Button(const char* a_label) const
		{
			return _button(a_label, Vec2{ 0.0F, 0.0F });
		}

		bool Checkbox(const char* a_label, bool* a_value) const
		{
			return _checkbox(a_label, a_value);
		}

		bool SliderFloat(const char* a_label, float* a_value, float a_min, float a_max) const
		{
			return _sliderFloat(a_label, a_value, a_min, a_max, "%.1f", 0);
		}

		void SameLine() const
		{
			_sameLine(0.0F, -1.0F);
		}

		void Text(const char* a_text) const
		{
			_textUnformatted(a_text, nullptr);
		}

	private:
		template <class T>
		T GetFunction(const char* a_name) const
		{
			return reinterpret_cast<T>(GetProcAddress(_module, a_name));
		}

		using GetApiVersionFunction = std::uint32_t(*)();
		using AddSectionItemFunction = void(*)(const char*, RenderCallback);
		using RegisterInputFunction = std::int64_t(*)(InputCallback);
		using RegisterHudFunction = std::int64_t(*)(HudCallback);
		using ButtonFunction = bool(*)(const char*, Vec2);
		using CheckboxFunction = bool(*)(const char*, bool*);
		using SliderFloatFunction = bool(*)(const char*, float*, float, float, const char*, int);
		using SameLineFunction = void(*)(float, float);
		using TextUnformattedFunction = void(*)(const char*, const char*);

		HMODULE _module{ nullptr };
		GetApiVersionFunction _getApiVersion{ nullptr };
		AddSectionItemFunction _addSectionItem{ nullptr };
		RegisterInputFunction _registerInput{ nullptr };
		RegisterHudFunction _registerHud{ nullptr };
		ButtonFunction _button{ nullptr };
		CheckboxFunction _checkbox{ nullptr };
		SliderFloatFunction _sliderFloat{ nullptr };
		SameLineFunction _sameLine{ nullptr };
		TextUnformattedFunction _textUnformatted{ nullptr };
	};

	inline Api& Get()
	{
		static Api api;
		return api;
	}
}
