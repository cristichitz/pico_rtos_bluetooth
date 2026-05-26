/**
 * @file security_event_handler.cpp
 * @brief Implementation file for the Security Manager callback logic used by
 * the project.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * It contains task-specific TODO notes that indicate the required
 * implementation points.
 *
 * This file is responsible for the application-side reaction to Security
 * Manager events. In the project, that means implementing logic that:
 * - logs or otherwise reports the pairing flow,
 * - confirms the supported pairing procedures when user interaction is
 *   required,
 * - reacts to pairing completion and re-encryption completion, and
 * - grants or denies authorization according to the chosen project policy.
 *
 * The project may organize this logic differently, but the same runtime
 * security behavior is still required.
 */

#include "security_event_handler.hpp"

void SecurityEventHandler::OnJustWorksRequest(c7222::ConnectionHandle con_handle) const {
	std::printf("just Works request: handle=0x%04x\n", con_handle);

	if(security_manager_ != nullptr) {
		(void)security_manager_->ConfirmJustWorks(con_handle);
	}

}

void SecurityEventHandler::OnNumericComparisonRequest(c7222::ConnectionHandle con_handle, uint32_t number) const {
	std::printf("numeric comparison: handle=0x%04x number=%lu\n", con_handle, static_cast<unsigned long>(number));

	if(security_manager_ != nullptr) {

		(void)security_manager_->ConfirmNumericComparison(con_handle, true);
	}
}

void SecurityEventHandler::OnPasskeyDisplay(c7222::ConnectionHandle con_handle, uint32_t passkey) const {
	std::printf("passkey display: handle=0x%04x passkey=%06lu\n", con_handle,static_cast<unsigned long>(passkey));
}

void SecurityEventHandler::OnPasskeyInput(c7222::ConnectionHandle con_handle) const {
	std::printf("passkey input requested: handle=0x%04x\n", con_handle);

	if(security_manager_ != nullptr) {
		(void)security_manager_->ProvidePasskey(con_handle, 123456);
	}
}

void SecurityEventHandler::OnPairingComplete(c7222::ConnectionHandle con_handle, c7222::SecurityManager::PairingStatus status, uint8_t status_code) const {

	std::printf("pairing complete: handle=0x%04x status=%u code=0x%02x\n", con_handle, static_cast<unsigned>(status), static_cast<unsigned>(status_code));

	// We pre-grant authorization after successful pairing.
	// BTstack's att_server checks gap_authorization_state() when a client
	// writes to a WRITE_AUTHORIZED characteristic. If the state is still
	// AUTHORIZATION_UNKNOWN, it loops endlessly calling sm_request_pairing().
	// Granting here ensures the state is AUTHORIZATION_GRANTED before the
	// // next write arrives.
	// We couldn't find any implementation of a trigger for the OnAuthorizationRequest event handler
	if(status == c7222::SecurityManager::PairingStatus::kSuccess && security_manager_ != nullptr) {
		std::printf("granting authorization for handle=0x%04x\n", con_handle);
		(void)security_manager_->SetAuthorization(con_handle, c7222::SecurityManager::AuthorizationResult::kGranted);
	}
}

void SecurityEventHandler::OnReencryptionComplete(c7222::ConnectionHandle con_handle, uint8_t status) const {
	std::printf("re-encryption complete: handle=0x%04x status=0x%02x\n", con_handle,static_cast<unsigned>(status));
}

void SecurityEventHandler::OnAuthorizationRequest(c7222::ConnectionHandle con_handle) const {

	std::printf("authorization request: handle=0x%04x\n", con_handle);

	if(security_manager_ != nullptr) {
		(void)security_manager_->SetAuthorization(con_handle, c7222::SecurityManager::AuthorizationResult::kGranted);
	}
}

void SecurityEventHandler::OnAuthorizationResult(
	c7222::ConnectionHandle con_handle, c7222::SecurityManager::AuthorizationResult result) const {
	std::printf("authorization result: handle=0x%04x result=%u\n", con_handle, static_cast<unsigned>(result));
}

