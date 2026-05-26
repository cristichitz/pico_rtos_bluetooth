/**
 * @file immediate_alert_service.cpp
 * @brief Reference implementation file for one possible Immediate Alert
 * Service design.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * It contains task-specific TODO notes that indicate the required
 * implementation points.
 *
 * This file documents one possible design for the Immediate Alert Service
 * component of the project. The main idea is to encapsulate all IAS-specific
 * logic into one class that owns the service object and its characteristics.
 *
 * An implementation corresponding to this file is expected to handle the
 * following responsibilities:
 * - resolve the Alert Level characteristic from the parsed IAS service,
 * - attach the write callback used for client writes,
 * - validate the written Alert Level value,
 * - map the accepted values to the required PWM duty cycles,
 * - update the stored characteristic value when the alert level changes, and
 * - reset the IAS-visible state on connection and disconnection when the
 *   chosen design requires it.
 *
 * The project does not require these responsibilities to be implemented as
 * member functions of an `ImmediateAlertService` class. The example class
 * design is only one possible organization of that behavior.
 */

#include "immediate_alert_service.hpp"

#include <cassert>
#include <cstdio>

#include "uuid.hpp"

ImmediateAlertService::ImmediateAlertService(c7222::Service* service, c7222::PwmOut* alert_pwm)
	: service_(service), alert_pwm_(alert_pwm) {}


void ImmediateAlertService::AlertLevelEventHandler::OnWrite(const std::vector<uint8_t>& data) {
	if(owner_ != nullptr) {
		owner_->HandleAlertLevelWrite(data);
	}
}


bool ImmediateAlertService::ResolveCharacteristic() {
	if(service_ == nullptr) {
		std::printf("service pointer is null\n");
		return false;
	}

	alert_level_characteristic_ = service_->FindCharacteristicByUuid(c7222::Uuid(module10_ias_spec::kAlertLevelUuid));

	if(alert_level_characteristic_ == nullptr) {
		std::printf("alert Level characteristic was not found\n");
		return false;
	}


	return true;
}

bool ImmediateAlertService::Initialize() {
	if(!ResolveCharacteristic()) {
		return false;
	}

	if(alert_level_characteristic_->HasUserDescription()) {
		(void)alert_level_characteristic_->SetUserDescriptionText("Alert Level");
	}

	alert_level_characteristic_->AddEventHandler(alert_level_event_handler_);
	SetAlertLevel(module10_ias_spec::AlertLevel::kNoAlert);
	return true;
}


void ImmediateAlertService::OnConnected() {
	SetAlertLevel(module10_ias_spec::AlertLevel::kNoAlert);
}

void ImmediateAlertService::OnDisconnected() {
	SetAlertLevel(module10_ias_spec::AlertLevel::kNoAlert);
}



void ImmediateAlertService::HandleAlertLevelWrite(const std::vector<uint8_t>& data) {
	if(data.empty()) {
		return;
	}

	const uint8_t raw_level = data[0];

	switch(raw_level) {
		case static_cast<uint8_t>(module10_ias_spec::AlertLevel::kNoAlert):
			SetAlertLevel(module10_ias_spec::AlertLevel::kNoAlert);
			break;

		case static_cast<uint8_t>(module10_ias_spec::AlertLevel::kMildAlert):
			SetAlertLevel(module10_ias_spec::AlertLevel::kMildAlert);
			break;

		case static_cast<uint8_t>(module10_ias_spec::AlertLevel::kHighAlert):
			SetAlertLevel(module10_ias_spec::AlertLevel::kHighAlert);
			break;

		default:
			std::printf("invalid alert level value: %u\n", raw_level);

			if(alert_level_characteristic_ != nullptr) {
				std::vector<uint8_t> valid_value{static_cast<uint8_t>(alert_level_)};
				(void)alert_level_characteristic_->GetValueAttribute().SetValue(valid_value);
			}
			break;
	}
}




void ImmediateAlertService::SetAlertLevel(module10_ias_spec::AlertLevel level) {
	alert_level_ = level;


	if(alert_pwm_ != nullptr) {
		alert_pwm_->Enable(true);
		alert_pwm_->SetDutyCycle(AlertLevelToDutyCycle(level));
	}

	if(alert_level_characteristic_ != nullptr) {
		std::vector<uint8_t> value{static_cast<uint8_t>(level)};
		(void)alert_level_characteristic_->GetValueAttribute().SetValue(value);
	}


	std::printf("alert level=%u\n", static_cast<unsigned>(level));
}

float ImmediateAlertService::AlertLevelToDutyCycle(module10_ias_spec::AlertLevel level) const {

	switch(level) {
		case module10_ias_spec::AlertLevel::kNoAlert:
			return 0.0f;

		case module10_ias_spec::AlertLevel::kMildAlert:
			return 0.25f;

		case module10_ias_spec::AlertLevel::kHighAlert:
			return 0.90f;

		default:
			assert(false && "unexpected IAS alert");
			return 0.0f;
	}
}


