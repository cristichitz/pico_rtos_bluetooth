/**
 * @file security_event_handler.hpp
 * @brief Template declaration for one possible Module 10 Pico BLE Security
 * Manager handler design.
 *
 * DISCLAIMER:
 * This file is a task template for ELEC C7222 Module 10 Task 10.1.
 * It contains task-specific TODO notes that indicate the required
 * implementation points.
 */
#ifndef EXAMPLES_BLE_PROJECT_SECURITY_EVENT_HANDLER_HPP_
#define EXAMPLES_BLE_PROJECT_SECURITY_EVENT_HANDLER_HPP_

#include <cstdint>
#include <cstdio>

#include "security_manager.hpp"

/**
 * @brief Example adapter for the Security Manager callbacks used in the
 * project.
 *
 * This class shows one possible way to keep pairing and authorization related
 * callbacks separate from the main application task. The project does not
 * require this exact class structure, but the design makes the security flow
 * explicit and keeps the callback implementations close to the
 * @ref c7222::SecurityManager object they operate on.
 *
 * In the course progression, this follows the same event-handler separation
 * used in earlier modules. Module 7, "GAP on Pico 2 W", introduces dedicated
 * GAP callback helpers, Module 8, "GATT on Pico 2 W", continues the same idea
 * with GAP and characteristic event handlers, and Module 9, "BLE Security on
 * Pico 2 W", applies the same pattern directly to Security Manager events.
 *
 * In this design, the object contains:
 * - a pointer to the enabled SecurityManager instance,
 * - callback overrides for the pairing procedures used by the project, and
 * - callback overrides for authorization and re-encryption reporting.
 *
 * The object may be default-constructed before BLE security is enabled.
 * After @ref c7222::Ble::EnableSecurityManager returns the runtime
 * SecurityManager object, that pointer is stored through the constructor or
 * @ref SetSecurityManager. The callback implementations can then confirm
 * pairing steps or grant authorization through that stored pointer.
 */
class SecurityEventHandler final : public c7222::SecurityManager::EventHandler {
   public:
	/**
	 * @brief Construct the handler with an optional SecurityManager pointer.
	 *
	 * @param security_manager SecurityManager instance used later by the
	 * callback implementations to answer pairing and authorization requests.
	 */
	explicit SecurityEventHandler(c7222::SecurityManager* security_manager)
		: security_manager_(security_manager) {}
		
	/** @brief Construct the handler without a bound SecurityManager. */
	SecurityEventHandler() = default;

	/**
	 * @brief Bind the SecurityManager used by the callback implementations.
	 *
	 * This setter supports the common initialization order where the event
	 * handler object is created first and the SecurityManager object becomes
	 * available only after BLE security is enabled.
	 *
	 * @param security_manager SecurityManager instance that owns the pairing
	 * procedures.
	 */
	void SetSecurityManager(c7222::SecurityManager* security_manager) {
		security_manager_ = security_manager;
	}


	void OnJustWorksRequest(c7222::ConnectionHandle con_handle) const override;
	void OnNumericComparisonRequest(c7222::ConnectionHandle con_handle, uint32_t number) const override;
	void OnPasskeyDisplay(c7222::ConnectionHandle con_handle, uint32_t passkey) const override;
	void OnPasskeyInput(c7222::ConnectionHandle con_handle) const override;

	void OnPairingComplete(c7222::ConnectionHandle con_handle, c7222::SecurityManager::PairingStatus status, uint8_t status_code) const override;
	void OnReencryptionComplete(c7222::ConnectionHandle con_handle, uint8_t status) const override;
	void OnAuthorizationRequest(c7222::ConnectionHandle con_handle) const override;

	void OnAuthorizationResult(c7222::ConnectionHandle con_handle, c7222::SecurityManager::AuthorizationResult result) const override;

   private:
	/** @brief Enabled SecurityManager used by the callback implementations. */
	c7222::SecurityManager* security_manager_ = nullptr;
};

#endif  // EXAMPLES_BLE_PROJECT_SECURITY_EVENT_HANDLER_HPP_
