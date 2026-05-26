/**
 * @file immediate_alert_service.hpp
 * @brief Shared Immediate Alert Service values and one reference class design.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * The example class-based solution is provided only as a reference design.
 */
#pragma once

#include <cstdint>
#include <vector>

#include "characteristic.hpp"
#include "pwm.hpp"
#include "service.hpp"

/**
 * @brief Shared Immediate Alert Service values available to all solutions.
 *
 * These constants and enum values are kept visible even when the reference
 * class-based design is hidden. They allow implementations to use the correct
 * Bluetooth-assigned values without copying raw numbers from the
 * specification.
 */
namespace module10_ias_spec {

/** @brief Immediate Alert Service UUID. */
inline constexpr uint16_t kServiceUuid = 0x1802;
/** @brief Alert Level characteristic UUID. */
inline constexpr uint16_t kAlertLevelUuid = 0x2A06;

/**
 * @brief Standard IAS Alert Level values.
 *
 * The numeric values match the Bluetooth specification, so they can be used
 * directly in characteristic payload handling.
 */
enum class AlertLevel : uint8_t {
	kNoAlert = 0,  /**< Disable the alert output. */
	kMildAlert = 1,  /**< Apply a low-intensity alert output. */
	kHighAlert = 2,  /**< Apply a high-intensity alert output. */
};

}  // namespace module10_ias_spec

/**
 * @brief Forward declaration for the reference IAS helper design.
 *
 * The full class declaration is kept in the solution section because this
 * object-oriented decomposition is an example, not a required project
 * architecture.
 */
class ImmediateAlertService;

/**
 * @brief Reference helper that encapsulates IAS runtime behavior.
 */
class ImmediateAlertService {
   public:
	/**
	 * @brief Construct the IAS helper from the resolved service and PWM output.
	 */
	explicit ImmediateAlertService(c7222::Service* service = nullptr, c7222::PwmOut* alert_pwm = nullptr);

	/**
	 * @brief Resolve characteristics, register callbacks, and initialize state.
	 */
	bool Initialize();

	/** @brief Reset IAS-visible state on new connection. */
	void OnConnected();
	/** @brief Reset IAS-visible state after disconnection. */
	void OnDisconnected();

   private:
	class AlertLevelEventHandler : public c7222::Characteristic::EventHandler {
	   public:
		explicit AlertLevelEventHandler(ImmediateAlertService* owner): owner_(owner) {}
		void OnWrite(const std::vector<uint8_t>& data) override;

	   private:
		ImmediateAlertService* owner_ = nullptr;
	};


	bool ResolveCharacteristic();
	void HandleAlertLevelWrite(const std::vector<uint8_t>& data);
	void SetAlertLevel(module10_ias_spec::AlertLevel level);
	float AlertLevelToDutyCycle(module10_ias_spec::AlertLevel level) const;

	c7222::Service* service_ = nullptr;
	c7222::PwmOut* alert_pwm_ = nullptr;
	c7222::Characteristic* alert_level_characteristic_ = nullptr;

	module10_ias_spec::AlertLevel alert_level_ = module10_ias_spec::AlertLevel::kNoAlert;
	AlertLevelEventHandler alert_level_event_handler_{this};
};


