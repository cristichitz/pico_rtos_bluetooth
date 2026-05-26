/**
 * @file gap_event_handler.hpp
 * @brief Template declaration for one possible Module 10 Pico BLE GAP event
 * handler design.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * It contains task-specific TODO notes that indicate the required
 * implementation points.
 */
#pragma once

#include "attribute_server.hpp"
#include "gap.hpp"

/**
 * @brief Example bridge between asynchronous GAP callbacks and the project
 * application logic.
 *
 * This class represents one possible design for tracking link state in the
 * project. It is intentionally small: it does not implement service logic,
 * but only translates GAP connection events into application-visible state.
 *
 * This is the same GAP-side bridge pattern used earlier in the course:
 * Module 7, "GAP on Pico 2 W", introduces the GAP event-handler structure for
 * advertising, connection, and disconnection flow; Module 8, "GATT on Pico 2
 * W", reuses it to connect GAP events to the AttributeServer; and Module 9,
 * "BLE Security on Pico 2 W", keeps the same event-handler structure while
 * adding Security Manager related initialization around the secured server.
 *
 * In this design, the object:
 * - stores a pointer to the GAP interface so advertising can be restarted,
 * - stores a pointer to the AttributeServer so the active connection handle
 *   can be propagated after connection, and
 * - caches a simple connected/disconnected flag for the main application
 *   task.
 *
 * The object may be constructed before the GATT database is enabled. In that
 * case, the AttributeServer pointer is injected later through
 * @ref SetAttributeServer once the parsed database has been enabled and the
 * server object is available.
 */
class GapEventHandler : public c7222::Gap::EventHandler {
   public:
	/**
	 * @brief Construct the GAP event handler.
	 *
	 * @param gap GAP instance used to restart advertising after disconnect.
	 * @param attribute_server Optional AttributeServer used to store the active
	 * connection handle after the GATT database has been enabled.
	 */
	explicit GapEventHandler(c7222::Gap* gap = c7222::Gap::GetInstance(), c7222::AttributeServer* attribute_server = nullptr);

	/**
	 * @brief Store the AttributeServer used for connection tracking.
	 *
	 * This setter supports the common project initialization order where the
	 * GAP event handler is constructed early, but the AttributeServer becomes
	 * available only after the parsed GATT database has been enabled.
	 *
	 * @param attribute_server AttributeServer associated with the BLE server.
	 */
	void SetAttributeServer(c7222::AttributeServer* attribute_server);

	/**
	 * @brief Query whether the server currently has an active connection.
	 *
	 * @return True when the latest GAP event indicates a connected state.
	 */
	bool IsConnected() const { return connected_; }

	void OnConnectionComplete(uint8_t status, c7222::ConnectionHandle con_handle, const c7222::BleAddress& address, uint16_t conn_interval, uint16_t conn_latency, uint16_t supervision_timeout) const override;
	void OnDisconnectionComplete(uint8_t status, c7222::ConnectionHandle con_handle, uint8_t reason) const override;

	
   private:
	/** @brief GAP instance used to restart advertising after disconnect. */
	c7222::Gap* gap_ = nullptr;
	/** @brief AttributeServer updated with the active connection handle. */
	c7222::AttributeServer* attribute_server_ = nullptr;
	/** @brief Cached connection flag updated by GAP callbacks. */
	mutable bool connected_ = false;
};
