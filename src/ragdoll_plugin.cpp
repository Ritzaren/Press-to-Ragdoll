#include "log.h"
#include "menu_framework_api.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace
{
	constexpr std::uint32_t kDefaultKeyboardKey = 0x3C;
	constexpr std::uint32_t kDefaultGamepadKey = 270;
	constexpr float kDefaultPushStrength = 100.0F;
	constexpr float kMinimumPushStrength = 1.0F;
	constexpr float kMaximumPushStrength = 100.0F;

	class RagdollController final : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		enum class BindingTarget
		{
			kNone,
			kKeyboard,
			kGamepad
		};

		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
			RE::BSTEventSource<RE::InputEvent*>*) override
		{
			if (bindingTarget != BindingTarget::kNone || !a_event || !*a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			for (auto event = *a_event; event; event = event->next) {
				const auto button = event->AsButtonEvent();
				if (!button || !button->IsDown()) {
					continue;
				}

				const auto device = button->GetDevice();
				const auto key = button->GetIDCode();
				if ((device == RE::INPUT_DEVICE::kKeyboard && key == keyboardKey) ||
					(device == RE::INPUT_DEVICE::kGamepad && key == gamepadKey)) {
					PushPlayerAway();
					break;
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		void LoadSettings()
		{
			auto path = SKSE::log::log_directory();
			if (!path) {
				return;
			}
			*path /= "PressToRagdoll.ini";
			const auto iniPath = path->string();
			keyboardKey = GetPrivateProfileIntA("Controls", "KeyboardKey", kDefaultKeyboardKey, iniPath.c_str());
			gamepadKey = GetPrivateProfileIntA("Controls", "GamepadKey", kDefaultGamepadKey, iniPath.c_str());
			char value[32]{};
			GetPrivateProfileStringA("General", "PushStrength", "100", value, sizeof(value), iniPath.c_str());
			char* end{};
			const auto parsedStrength = std::strtof(value, &end);
			if (end != value && std::isfinite(parsedStrength)) {
				pushStrength = std::clamp(parsedStrength, kMinimumPushStrength, kMaximumPushStrength);
			}
		}

		void RenderMenu()
		{
			auto& menu = MenuFramework::Get();
			menu.Text("Keyboard ragdoll key");
			menu.SameLine();
			if (menu.Button(bindingTarget == BindingTarget::kKeyboard ? "Press keyboard key..." : "Bind keyboard key")) {
				bindingTarget = BindingTarget::kKeyboard;
			}
			menu.Text(("Keyboard key code: " + std::to_string(keyboardKey)).c_str());

			menu.Text("Gamepad ragdoll key");
			menu.SameLine();
			if (menu.Button(bindingTarget == BindingTarget::kGamepad ? "Press gamepad button..." : "Bind gamepad button")) {
				bindingTarget = BindingTarget::kGamepad;
			}
			menu.Text(("Gamepad key code: " + std::to_string(gamepadKey)).c_str());
			if (menu.SliderFloat("Ragdoll force", &pushStrength, kMinimumPushStrength, kMaximumPushStrength)) {
				SavePushStrength();
			}

			if (bindingTarget != BindingTarget::kNone) {
				menu.Text("Press the selected key or button. Escape cancels keyboard binding.");
				if (menu.Button("Cancel binding")) {
					bindingTarget = BindingTarget::kNone;
				}
			}
		}

		bool CaptureBinding(RE::InputEvent* a_event)
		{
			if (bindingTarget == BindingTarget::kNone || !a_event) {
				return false;
			}

			const auto button = a_event->AsButtonEvent();
			if (!button || !button->IsDown()) {
				return false;
			}

			const auto device = button->GetDevice();
			const auto key = button->GetIDCode();
			if (bindingTarget == BindingTarget::kKeyboard && device == RE::INPUT_DEVICE::kKeyboard) {
				if (key == 1) {
					bindingTarget = BindingTarget::kNone;
					return true;
				}
				keyboardKey = key;
				SaveSetting("KeyboardKey", keyboardKey);
			} else if (bindingTarget == BindingTarget::kGamepad && device == RE::INPUT_DEVICE::kGamepad) {
				gamepadKey = key;
				SaveSetting("GamepadKey", gamepadKey);
			} else {
				return false;
			}

			bindingTarget = BindingTarget::kNone;
			return true;
		}

		void SavePushStrength()
		{
			if (auto path = SKSE::log::log_directory()) {
				*path /= "PressToRagdoll.ini";
				const auto iniPath = path->string();
				const auto value = std::to_string(pushStrength);
				WritePrivateProfileStringA("General", "PushStrength", value.c_str(), iniPath.c_str());
			}
		}

		void RegisterInput()
		{
			if (const auto input = RE::BSInputDeviceManager::GetSingleton()) {
				input->AddEventSink(this);
			}
		}

	private:
		void SaveSetting(const char* a_key, std::uint32_t a_value)
		{
			if (auto path = SKSE::log::log_directory()) {
				*path /= "PressToRagdoll.ini";
				const auto iniPath = path->string();
				const auto value = std::to_string(a_value);
				WritePrivateProfileStringA("Controls", a_key, value.c_str(), iniPath.c_str());
			}
		}

		void PushPlayerAway()
		{
			auto player = RE::PlayerCharacter::GetSingleton();
			auto skyrimVM = RE::SkyrimVM::GetSingleton();
			if (!player || !skyrimVM) {
				return;
			}

			auto& vm = skyrimVM->GetVMRuntimeData().impl;
			if (!vm) {
				return;
			}

			const auto policy = vm->GetObjectHandlePolicy();
			if (!policy) {
				return;
			}

			const auto handle = policy->GetHandleForObject(RE::FormType::ActorCharacter, player);
			RE::BSTSmartPointer<RE::BSScript::Object> playerObject;
			if (!vm->FindBoundObject(handle, "Actor", playerObject)) {
				return;
			}

			auto arguments = std::unique_ptr<RE::BSScript::IFunctionArguments>(
				RE::MakeFunctionArguments(static_cast<RE::Actor*>(player), static_cast<float>(pushStrength)));
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result;
			vm->DispatchMethodCall(playerObject, RE::BSFixedString("PushActorAway"), arguments.get(), result);
		}

		std::uint32_t keyboardKey{ kDefaultKeyboardKey };
		std::uint32_t gamepadKey{ kDefaultGamepadKey };
		float pushStrength{ kDefaultPushStrength };
		BindingTarget bindingTarget{ BindingTarget::kNone };
	};

	RagdollController g_ragdollController;
	std::int64_t g_inputRegistration{ 0 };

	void __stdcall RenderMenu()
	{
		g_ragdollController.RenderMenu();
	}

	bool __stdcall CaptureMenuInput(RE::InputEvent* a_event)
	{
		return g_ragdollController.CaptureBinding(a_event);
	}

	void RegisterMenu()
	{
		auto& framework = MenuFramework::Get();
		if (!framework.Initialize()) {
			spdlog::warn("SKSE Menu Framework API unavailable; Press to Ragdoll settings menu disabled");
			return;
		}

		framework.AddSectionItem("Press to Ragdoll/Controls", RenderMenu);
		g_inputRegistration = framework.RegisterInput(CaptureMenuInput);
	}

	void MessageHandler(SKSE::MessagingInterface::Message* a_message)
	{
		if (!a_message) {
			return;
		}

		switch (a_message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			g_ragdollController.LoadSettings();
			g_ragdollController.RegisterInput();
			break;
		case SKSE::MessagingInterface::kPostPostLoad:
			RegisterMenu();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();

	const auto messaging = SKSE::GetMessagingInterface();
	return messaging && messaging->RegisterListener("SKSE", MessageHandler);
}
