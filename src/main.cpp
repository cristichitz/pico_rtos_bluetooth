/**
 * @file main.cpp
 * @brief Example top-level application flow for the Module 10 secure BLE
 * project.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * It contains task-specific TODO notes that indicate the required
 * implementation points.
 *
 * This file documents one possible orchestration design for the project. The
 * main idea is to keep responsibilities separated:
 * - startup and task creation stay in @ref main,
 * - BLE stack bring-up is handled in @ref BleTask,
 * - advertising and connection handling are delegated to the GAP layer and
 *   its event handler,
 * - security setup is performed before the secured Attribute Server is
 *   enabled,
 * - service objects are resolved from the parsed GATT database after
 *   @ref c7222::Ble::EnableAttributeServer returns, and
 * - board events such as button presses are converted into BLE-visible
 *   service behavior.
 *
 * This is the same high-level progression used in the earlier Pico BLE
 * modules:
 * - Module 7 established the GAP startup and event-handler pattern,
 * - Module 8 added GATT database enable, service lookup, and characteristic
 *   event handling, and
 * - Module 9 inserted Security Manager setup before enabling a secured
 *   Attribute Server.
 *
 * The project does not require this exact function decomposition. It is an
 * example application structure that makes the runtime order explicit.
 */

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>

#include "advertisement_data.hpp"
#include "attribute_server.hpp"
#include "ble.hpp"
#include "button.hpp"
#include "characteristic.hpp"
#include "c7222_pico_w_board.hpp"
#include "freertos_event_group.hpp"
#include "freertos_task.hpp"
#include "gap.hpp"
#include "led.hpp"
#include "platform.hpp"
#include "pwm.hpp"
#include "security_manager.hpp"
#include "service.hpp"
#include "uuid.hpp"

#include "alert_notification_service.hpp"
#include "app_profile.h"
#include "gap_event_handler.hpp"
#include "immediate_alert_service.hpp"
#include "security_event_handler.hpp"

namespace {

/** @brief BLE device name required by the project brief. */
constexpr const char* kDeviceName = "FindMe-7";
/** @brief Event-group bit raised when the user button is pressed. */
constexpr uint32_t kButtonPressedEventMask = (1u << 0);

/**
 * @brief Small container that groups the resolved project services.
 *
 * The parsed GATT database exposes services through the AttributeServer after
 * the profile has been enabled. This helper struct collects the resolved
 * service pointers needed by the example application flow.
 */
struct ProjectGattObjects {
	/** @brief Resolved Immediate Alert Service instance. */
	c7222::Service* immediate_alert_service = nullptr;
	/** @brief Resolved Alert Notification Service instance. */
	c7222::Service* alert_notification_service = nullptr;
};

/** @brief Active AttributeServer pointer after the GATT database is enabled. */
static c7222::AttributeServer* g_att_server = nullptr;
/** @brief PWM object used to visualize the IAS Alert Level. */
static std::unique_ptr<c7222::PwmOut> g_alert_pwm;
/** @brief Status LED that blinks while advertising and stays on while connected. */
static c7222::Led* g_status_led = nullptr;
/** @brief User button source for ANS updates. */
static c7222::Button* g_button = nullptr;
/** @brief Event group used to move button IRQ events into the BLE task. */
static c7222::FreeRtosEventGroup g_event_group;
/** @brief FreeRTOS task object that runs the BLE application loop. */
static c7222::FreeRtosTask g_ble_task;

/** @brief GAP callback helper used by the example application flow. */
static GapEventHandler g_gap_event_handler;
/** @brief Security Manager callback helper used by the example flow. */
static SecurityEventHandler g_security_event_handler;

/**
 * @brief Configure GAP advertising for the project device.
 *
 * This helper exists to isolate all advertising-related setup in one place.
 * In the example design it performs four jobs:
 * - acquires the GAP interface from the BLE facade,
 * - registers the shared GAP event handler,
 * - configures the advertising payload metadata such as flags and device
 *   name, and
 * - starts connectable advertising.
 *
 * The function is called from @ref OnBleStackOn so GAP operations start only
 * after the BLE stack reports that it is ready.
 */
void ConfigureAdvertisement() {
	auto* ble = c7222::Ble::GetInstance();
	auto* gap = ble->GetGap();

	gap->AddEventHandler(g_gap_event_handler);
	ble->SetAdvertisementFlags(c7222::AdvertisementData::Flags::kLeGeneralDiscoverableMode | c7222::AdvertisementData::Flags::kBrEdrNotSupported);
	ble->SetDeviceName(kDeviceName);


	c7222::Gap::AdvertisementParameters params;
	
	params.advertising_type = c7222::Gap::AdvertisingType::kAdvInd;
	params.min_interval = 320;
	params.max_interval = 400;
	gap->SetAdvertisingParameters(params);
	gap->StartAdvertising();

	std::printf("advertising started '%s'\n", kDeviceName);
}

/**
 * @brief Enable and configure the Security Manager for the secured server.
 *
 * This helper exists because the project uses authenticated and encrypted
 * attributes. In the example flow it is called before
 * @ref c7222::Ble::EnableAttributeServer, which mirrors the secured-server
 * ordering introduced in Module 9.
 *
 * The function is responsible for:
 * - selecting the desired pairing and authentication policy,
 * - enabling the runtime Security Manager object,
 * - injecting that object into the shared security event handler, and
 * - registering the security event handler with the BLE facade.
 *
 * @param ble BLE facade used to access the Security Manager subsystem.
 */
void ConfigureSecurityManager(c7222::Ble* ble) {
	assert(ble != nullptr);

	c7222::SecurityManager::SecurityParameters security_params;
	security_params.io_capability = c7222::SecurityManager::IoCapability::kDisplayYesNo;
	security_params.authentication = c7222::SecurityManager::AuthenticationRequirement::kMitmProtection;
	security_params.gatt_client_required_security_level = c7222::SecurityManager::GattClientSecurityLevel::kLevel3;


	auto* security_manager = ble->EnableSecurityManager(security_params);
	if (!security_manager)
	{
		std::cout << "Security Enabling failed" << std::endl;
		
	}
	g_security_event_handler.SetSecurityManager(security_manager);
	ble->AddSecurityEventHandler(&g_security_event_handler);

}

/**
 * @brief Resolve the service objects needed by the project from the parsed
 * GATT database.
 *
 * Once the Attribute Server has been enabled, the compiled profile can be
 * queried through service UUIDs. This helper keeps that lookup separate from
 * the rest of the application task so the initialization flow stays readable.
 *
 * In the reference design, the returned service pointers are then passed to
 * service-specific helper objects such as the IAS and ANS example classes.
 * Other designs may use the same resolved objects differently.
 *
 * @return Struct containing the resolved IAS and ANS service pointers.
 */
ProjectGattObjects ResolveGattObjects() {
	ProjectGattObjects objects;

	if(g_att_server == nullptr) {
		return objects;
	}

	objects.immediate_alert_service = g_att_server->FindServiceByUuid(c7222::Uuid(module10_ias_spec::kServiceUuid));
	objects.alert_notification_service = g_att_server->FindServiceByUuid(c7222::Uuid(module10_ans_spec::kServiceUuid));

	return objects;
}

/**
 * @brief Configure the board-side outputs and inputs used by the project.
 *
 * This helper groups the physical board wiring decisions into one function.
 * In the example flow it performs two jobs:
 * - creates the PWM output used by the Immediate Alert Service, and
 * - converts the Pico button interrupt into an event-group bit that the BLE
 *   task can process outside interrupt context.
 *
 * The intent is to keep hardware event capture separate from BLE protocol
 * handling.
 */
void ConfigureBoardOutputs() {

	auto* platform = c7222::Platform::GetInstance();
	auto* board = platform->GetPicoWBoard();

	g_status_led = &board->GetLed(c7222::PicoWBoard::LedId::LED2_RED);
	g_button = &board->GetButton(c7222::PicoWBoard::ButtonId::BUTTON_B1);

	if(g_status_led != nullptr) {
		g_status_led->Off();
	}

	g_alert_pwm = platform->CreateLedPwm(c7222::PicoWBoard::LedId::LED1_GREEN, 0);

	if(g_alert_pwm != nullptr) {
		g_alert_pwm->SetDutyCycle(0.0f);
		g_alert_pwm->Enable(true);
	}

	if(g_button != nullptr) {
		g_button->DisableIrq();
		g_button->EnableIrq(c7222::GpioInputEvent::FallingEdge, [](std::uint32_t events) {
							   if(events & static_cast<std::uint32_t>(c7222::GpioInputEvent::FallingEdge)) {
								   (void)g_event_group.SetBitsFromISR(kButtonPressedEventMask);
							   }
		});
	}
}

/**
 * @brief Convert a captured button event into ANS behavior.
 *
 * This helper exists to keep the BLE task loop small. In the example design
 * it first checks whether the device is currently connected, because the
 * project counts alerts per active connection. If connected, it forwards the
 * event to the ANS helper object.
 *
 * @param connected Current connection state reported by the GAP handler.
 * @param alert_notification_service Application object that applies ANS logic.
 */
void HandleButtonPress(bool connected, AlertNotificationService& alert_notification_service) {
	if(!connected) {
		return;
	}

	alert_notification_service.RegisterSimpleAlertFromButton();
}



/**
 * @brief BLE-stack-ready callback used by the example startup flow.
 *
 * The BLE facade invokes this callback after the stack has turned on. The
 * example design uses it as the earliest safe place to configure and start
 * advertising.
 */
void OnBleStackOn() {
	ConfigureAdvertisement();

}

/**
 * @brief Main BLE application task for the reference design.
 *
 * This function demonstrates the full runtime order used by the example
 * application structure:
 * 1. acquire the BLE facade,
 * 2. enable security,
 * 3. enable the Attribute Server with the compiled profile,
 * 4. connect the GAP handler to the Attribute Server,
 * 5. configure board I/O,
 * 6. resolve the project services from the parsed GATT database,
 * 7. construct the application-side service objects,
 * 8. turn on the BLE stack, and
 * 9. run the main event loop.
 *
 * Inside the loop, the task has three responsibilities:
 * - observe connection-state transitions and notify the application objects,
 * - process button events delivered through the event group, and
 * - update the status LED while the device is not connected.
 *
 * @param params Unused FreeRTOS task parameter.
 */

[[noreturn]] void BleTask(void* /*params*/) {

	auto* ble = c7222::Ble::GetInstance(false);

	ConfigureSecurityManager(ble);
	g_att_server = ble->EnableAttributeServer(profile_data);

	if(g_att_server == nullptr) {
		std::printf("fail to enable attribute server\n");

		while(true) {
			c7222::FreeRtosTask::Delay(c7222::FreeRtosTask::MsToTicks(500));
		}
	}

	g_gap_event_handler.SetAttributeServer(g_att_server);
	ConfigureBoardOutputs();

	const ProjectGattObjects gatt_objects = ResolveGattObjects();

	if(gatt_objects.immediate_alert_service == nullptr || gatt_objects.alert_notification_service == nullptr) {

		std::printf("required project services were not found\n");

		while(true) {
			c7222::FreeRtosTask::Delay(c7222::FreeRtosTask::MsToTicks(500));
		}

	}

	ImmediateAlertService immediate_alert_service(gatt_objects.immediate_alert_service, g_alert_pwm.get());
	AlertNotificationService alert_notification_service(gatt_objects.alert_notification_service);


	if(!immediate_alert_service.Initialize() || !alert_notification_service.Initialize()) {
		std::printf("failed to init IAS/ANS helpers\n");

		while(true) {
			c7222::FreeRtosTask::Delay(c7222::FreeRtosTask::MsToTicks(500));
		}

	}

	ble->SetOnBleStackOnCallback(OnBleStackOn);
	ble->TurnOn();

	bool was_connected = false;
	bool status_led_blink_on = false;

	uint32_t last_blink_tick = c7222::FreeRtosTask::GetTickCount();
	const uint32_t blink_period_ticks = c7222::FreeRtosTask::MsToTicks(400);
	const uint32_t wait_ticks = c7222::FreeRtosTask::MsToTicks(100);

	while(true) {
		const uint32_t bits = g_event_group.WaitBits(kButtonPressedEventMask, true, false, wait_ticks);

		const bool connected = g_gap_event_handler.IsConnected();

		if(connected && !was_connected) {

			immediate_alert_service.OnConnected();
			alert_notification_service.OnConnected();

			if(g_status_led != nullptr) {
				g_status_led->On();
			}
		}

		if(!connected && was_connected) {
			immediate_alert_service.OnDisconnected();
			alert_notification_service.OnDisconnected();

			status_led_blink_on = false;

			last_blink_tick = c7222::FreeRtosTask::GetTickCount();

			if(g_status_led != nullptr) {

				g_status_led->Off();
			}

		}
		was_connected = connected;

		if((bits & kButtonPressedEventMask) != 0u) {
			HandleButtonPress(connected, alert_notification_service);
		}

		if(connected) {
			if(g_status_led != nullptr) {
				g_status_led->On();
			}

			continue;
		}



		const uint32_t now_tick = c7222::FreeRtosTask::GetTickCount();

		if((now_tick - last_blink_tick) >= blink_period_ticks) {
			last_blink_tick = now_tick;
			status_led_blink_on = !status_led_blink_on;

			if(g_status_led != nullptr) {
				if(status_led_blink_on) {
					g_status_led->On();
				} else {
					g_status_led->Off();
				}
			}
		}
	}
}

}  // namespace

/**
 * @brief Program entry point for the example project application.
 *
 * This function is intentionally small. Its purpose is to perform the minimum
 * platform startup needed before the scheduler takes control:
 * - initialize the Pico platform abstraction,
 * - create the BLE application task, and
 * - start the FreeRTOS scheduler.
 *
 * After the scheduler starts, the ongoing BLE behavior is driven by
 * @ref BleTask and the registered callback handlers.
 *
 * @return This function never returns.
 */
[[noreturn]] int main() {

	auto* platform = c7222::Platform::GetInstance();

	if(!platform->Initialize()) {
		std::printf("plat init failed\n");

		while(true) {}
	}


	if(!g_event_group.Initialize()) {
		std::printf("event group init failed\n");

		while(true) {}
	}


	if(!g_ble_task.Initialize("BLE_App", 1024, c7222::FreeRtosTask::IdlePriority() + 1u, BleTask, nullptr)) {
		std::printf("BLE task init fail\n");

		while(true) {}
	}

	c7222::FreeRtosTask::StartScheduler();


	while(true) {}
}
