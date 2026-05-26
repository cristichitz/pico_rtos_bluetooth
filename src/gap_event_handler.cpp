/**
 * @file gap_event_handler.cpp
 * @brief Implementation file for the GAP callback logic used by the project.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * It contains task-specific TODO notes that indicate the required
 * implementation points.
 *
 * This file is responsible for the application-side reaction to GAP events.
 * In the project, that means implementing logic that:
 * - stores the AttributeServer pointer once it becomes available,
 * - updates the current connection state when a link is established,
 * - propagates the active connection handle to the AttributeServer, and
 * - restores the advertising state after a disconnect.
 *
 * The project may organize this logic differently, but the same runtime
 * behavior is still required.
 */

#include "gap_event_handler.hpp"
#include <cstdio>


GapEventHandler::GapEventHandler(c7222::Gap* gap, c7222::AttributeServer* attribute_server)
	: gap_(gap), attribute_server_(attribute_server) {}


void GapEventHandler::SetAttributeServer(c7222::AttributeServer* attribute_server) {
	attribute_server_ = attribute_server;
}



void GapEventHandler::OnConnectionComplete(uint8_t status, c7222::ConnectionHandle con_handle, const c7222::BleAddress& /*address*/, uint16_t conn_interval, uint16_t conn_latency, uint16_t supervision_timeout) const {

	std::printf("Connected: status=0x%02X, handle=%u, interval=%u, latency=%u, timeout=%u\n",
				status,
				con_handle,
				conn_interval,
				conn_latency,
				supervision_timeout);


	if(status != 0) {
		return;
	}

	connected_ = true;
	if(attribute_server_ != nullptr) {
		attribute_server_->SetConnectionHandle(con_handle);
	}
}

void GapEventHandler::OnDisconnectionComplete(uint8_t status, c7222::ConnectionHandle con_handle, uint8_t reason) const {

	std::printf("disconnected: status=0x%02X, handle=%u, reason=0x%02X\n", status, con_handle, reason);

	connected_ = false;

	if(attribute_server_ != nullptr) {
		attribute_server_->SetDisconnected();
	}

	if(gap_ != nullptr) {
		gap_->StartAdvertising();
	}
}

