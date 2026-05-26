/**
 * @file alert_notification_service.cpp
 * @brief Reference implementation file for one possible Alert Notification
 * Service design.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * It contains task-specific TODO notes that indicate the required
 * implementation points.
 *
 * This file documents one possible design for the Alert Notification Service
 * component of the project. The main idea is to encapsulate all ANS-specific
 * logic into one class that owns the service object and its characteristics.
 *
 * An implementation corresponding to this file is expected to handle the
 * following responsibilities:
 * - resolve the ANS characteristics required by the project from the parsed
 *   service object,
 * - initialize the supported-category values exposed by the server,
 * - track unread counts and client-enabled category masks during a
 *   connection,
 * - react to Control Point writes and CCCD updates,
 * - generate the correct New Alert and Unread Alert Status payloads, and
 * - update or notify the corresponding characteristics when button-generated
 *   alerts occur.
 *
 * The project does not require these responsibilities to be implemented as
 * member functions of an `AlertNotificationService` class. The example class
 * design is only one possible organization of that behavior.
 */

#include "alert_notification_service.hpp"
#include <cassert>
#include <cstdio>
#include "uuid.hpp"


AlertNotificationService::AlertNotificationService(c7222::Service* service): service_(service) {}


void AlertNotificationService::ControlPointEventHandler::OnWrite(
	const std::vector<uint8_t>& data) {
	if(owner_ != nullptr) {
		owner_->HandleControlPointWrite(data);
	}
}



bool AlertNotificationService::ResolveCharacteristics() {

	if(service_ == nullptr) {
		std::printf("service pointer is null\n");
		return false;
	}


	supported_new_alert_category_characteristic_ = service_->FindCharacteristicByUuid(c7222::Uuid(module10_ans_spec::kSupportedNewAlertCategoryUuid));
	new_alert_characteristic_ = service_->FindCharacteristicByUuid(c7222::Uuid(module10_ans_spec::kNewAlertUuid));
	supported_unread_alert_category_characteristic_ = service_->FindCharacteristicByUuid(c7222::Uuid(module10_ans_spec::kSupportedUnreadAlertCategoryUuid));
	unread_alert_status_characteristic_ = service_->FindCharacteristicByUuid(c7222::Uuid(module10_ans_spec::kUnreadAlertStatusUuid));
	control_point_characteristic_ = service_->FindCharacteristicByUuid(c7222::Uuid(module10_ans_spec::kAlertNotificationControlPointUuid));



	if(supported_new_alert_category_characteristic_ == nullptr || new_alert_characteristic_ == nullptr || supported_unread_alert_category_characteristic_ == nullptr ||
	   unread_alert_status_characteristic_ == nullptr ||
	   control_point_characteristic_ == nullptr) {
		std::printf("required characteristics are missing\n");
		return false;
	}

	return true;
}

void AlertNotificationService::SetUserDescriptions() {

	if(supported_new_alert_category_characteristic_ != nullptr &&
	   supported_new_alert_category_characteristic_->HasUserDescription()) {
		(void)supported_new_alert_category_characteristic_->SetUserDescriptionText("Supported New Alert Category");
	}

	if(new_alert_characteristic_ != nullptr && new_alert_characteristic_->HasUserDescription()) {
		(void)new_alert_characteristic_->SetUserDescriptionText("New Alert");
	}

	if(supported_unread_alert_category_characteristic_ != nullptr &&
	   supported_unread_alert_category_characteristic_->HasUserDescription()) {
		(void)supported_unread_alert_category_characteristic_->SetUserDescriptionText("Supported Unread Alert Category");
	}

	if(unread_alert_status_characteristic_ != nullptr &&
	   unread_alert_status_characteristic_->HasUserDescription()) {
		(void)unread_alert_status_characteristic_->SetUserDescriptionText("Unread Alert Status");
	}

	if(control_point_characteristic_ != nullptr &&
	   control_point_characteristic_->HasUserDescription()) {
		(void)control_point_characteristic_->SetUserDescriptionText("Alert Notification Control Point");
	}

}

bool AlertNotificationService::Initialize() {

	if(!ResolveCharacteristics()) {
		
		return false;

	}


	SetUserDescriptions();
	control_point_characteristic_->AddEventHandler(control_point_event_handler_);

	const std::vector<uint8_t> supported_categories{static_cast<uint8_t>(module10_ans_spec::kSimpleAlertMask & 0xFFu), static_cast<uint8_t>((module10_ans_spec::kSimpleAlertMask >> 8) & 0xFFu)};
	
	SetCharacteristicValue(supported_new_alert_category_characteristic_, supported_categories, false);
	SetCharacteristicValue(supported_unread_alert_category_characteristic_, supported_categories, false);

	ResetConnectionState();
	return true;
}

void AlertNotificationService::OnConnected() {
	ResetConnectionState();
}

void AlertNotificationService::OnDisconnected() {
	ResetConnectionState();
}



void AlertNotificationService::ResetConnectionState() {
	new_alert_updates_enabled_ = false;
	unread_alert_status_updates_enabled_ = false;
	new_alert_count_ = 0;
	unread_alert_count_ = 0;

	UpdateNewAlertValue(false);
	UpdateUnreadAlertStatusValue(false);
}


void AlertNotificationService::RegisterSimpleAlertFromButton() {

	if(unread_alert_count_ < 0xFFu) {
		++unread_alert_count_;
	}
	new_alert_count_ = 1;

	UpdateNewAlertValue(false);
	UpdateUnreadAlertStatusValue(false);


	if(new_alert_updates_enabled_ && IsClientSubscribed(new_alert_characteristic_)) {
		UpdateNewAlertValue(true);
	}

	if(unread_alert_status_updates_enabled_ &&
	   IsClientSubscribed(unread_alert_status_characteristic_)) {
		UpdateUnreadAlertStatusValue(true);
	}


	std::printf("simple alert generated, unread count=%u\n",
				static_cast<unsigned>(unread_alert_count_));
}

void AlertNotificationService::HandleControlPointWrite(const std::vector<uint8_t>& data) {

	if(data.size() < 2) {
		std::printf("control point write too short\n");
		return;
	}

	const uint8_t command_raw = data[0];
	const uint8_t category_raw = data[1];


	if(command_raw >
	   static_cast<uint8_t>(module10_ans_spec::Command::kNotifyUnreadCategoryStatusImmediately)) {
		std::printf("unknown control point command: %u\n", command_raw);
		return;
	}


	if(!IsSimpleCategoryOrAll(category_raw)) {
		std::printf("category not supported: %u\n", category_raw);
		return;
	}

	HandleControlPointCommand(static_cast<module10_ans_spec::Command>(command_raw), category_raw);
}



void AlertNotificationService::HandleControlPointCommand(module10_ans_spec::Command command, uint8_t /*category_raw*/) {

	switch(command) {
		case module10_ans_spec::Command::kEnableNewIncomingAlertNotification:
			new_alert_updates_enabled_ = true;
			break;

		case module10_ans_spec::Command::kEnableUnreadCategoryStatusNotification:
			unread_alert_status_updates_enabled_ = true;
			break;

		case module10_ans_spec::Command::kDisableNewIncomingAlertNotification:
			new_alert_updates_enabled_ = false;
			break;

		case module10_ans_spec::Command::kDisableUnreadCategoryStatusNotification:
			unread_alert_status_updates_enabled_ = false;
			break;

		case module10_ans_spec::Command::kNotifyNewIncomingAlertImmediately:
			if(new_alert_updates_enabled_ && IsClientSubscribed(new_alert_characteristic_)) {
				UpdateNewAlertValue(true);
			}
			break;

		case module10_ans_spec::Command::kNotifyUnreadCategoryStatusImmediately:
			if(unread_alert_status_updates_enabled_ &&
			   IsClientSubscribed(unread_alert_status_characteristic_)) {
				UpdateUnreadAlertStatusValue(true);
			}
			break;

		default:
			assert(false && "unexpected ANS control command");
			break;
	}
}


bool AlertNotificationService::IsSimpleCategoryOrAll(uint8_t category_raw) const {
	return category_raw == static_cast<uint8_t>(module10_ans_spec::Category::kSimpleAlert) || category_raw == static_cast<uint8_t>(module10_ans_spec::Category::kAllAlerts);
}


bool AlertNotificationService::IsClientSubscribed(const c7222::Characteristic* characteristic) const {

	if(characteristic == nullptr || !characteristic->HasCCCD()) {
		return false;
	}

	return characteristic->IsNotificationsEnabled() || characteristic->IsIndicationsEnabled();
}


void AlertNotificationService::SetCharacteristicValue(c7222::Characteristic* characteristic, const std::vector<uint8_t>& value, bool send_update) {

	if(characteristic == nullptr) {
		return;
	}

	if(send_update) {
		(void)characteristic->SetValue(value);
		return;
	}

	(void)characteristic->GetValueAttribute().SetValue(value);
}


void AlertNotificationService::UpdateNewAlertValue(bool send_update) {
	SetCharacteristicValue(new_alert_characteristic_, BuildNewAlertValue(), send_update);
}


void AlertNotificationService::UpdateUnreadAlertStatusValue(bool send_update) {
	SetCharacteristicValue(
		unread_alert_status_characteristic_, BuildUnreadAlertStatusValue(), send_update);
}


std::vector<uint8_t> AlertNotificationService::BuildNewAlertValue() const {
	return {
		static_cast<uint8_t>(module10_ans_spec::Category::kSimpleAlert), new_alert_count_};

}



std::vector<uint8_t> AlertNotificationService::BuildUnreadAlertStatusValue() const {
	
	return {
		static_cast<uint8_t>(module10_ans_spec::Category::kSimpleAlert), unread_alert_count_};
}


