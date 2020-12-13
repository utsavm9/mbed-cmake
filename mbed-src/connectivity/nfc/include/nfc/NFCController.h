/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MBED_NFC_CONTROLLER_H
#define MBED_NFC_CONTROLLER_H

#include <stdint.h>
#include "events/EventQueue.h"
#include "platform/SharedPtr.h"
#include "drivers/Timer.h"
#include "drivers/Timeout.h"

#include "NFCDefinitions.h"
#include "NFCControllerDriver.h"

#include "platform/Span.h"

namespace mbed {
namespace nfc {

/** @addtogroup nfc
 * @{
 */

class NFCRemoteInitiator;
class NFCRemoteTarget;
class NFCControllerDriver;

/**
 * This class represents a NFC Controller.
 *
 * A controller can be in one of three different states:
 *  * Idle/sleep state
 *  * Discovery state: The controller tries to discover a remote endpoint (initiator or target)
 *  * Connected state: The controller exchanges data with an endpoint (initiator or target)
 *
 * A NFCController instance needs to be initialized with a NFCControllerDriver instance which abstracts the specific controller being used.
 * A delegate needs to be set by the user to receive discovery events.
 */
class NFCController : private NFCControllerDriver::Delegate {
public:

    /**
     * The NFCController delegate. Users of the NFCController class need to implement this delegate's methods to receive events.
     */
    struct Delegate {
        /**
         * A enumeration of causes for the discovery process terminating.
         */
        enum nfc_discovery_terminated_reason_t {
            nfc_discovery_terminated_completed = 0, ///< Process completed, at least one endpoint was discovered
            nfc_discovery_terminated_canceled, ///< Process was canceled by the user
            nfc_discovery_terminated_rf_error ///< An unexpected error was encountered during an exchange on the air interface
        };

        /**
         * The discovery process terminated.
         * @param[in] reason the cause for the termination
         */
        virtual void on_discovery_terminated(nfc_discovery_terminated_reason_t reason) {}

        /**
         * A remote initiator was discovered (the local controller is in target mode).
         * @param[in] nfc_initiator the NFCRemoteInitiator instance
         */
        virtual void on_nfc_initiator_discovered(const SharedPtr<NFCRemoteInitiator> &nfc_initiator) {}

        /**
         * A remote target was discovered (the local controller is in initiator mode).
         * @param[in] nfc_target the NFCRemoteTarget instance
         */
        virtual void on_nfc_target_discovered(const SharedPtr<NFCRemoteTarget> &nfc_target) {}

    protected:
        ~Delegate() { }
    };

    /**
     * Construct a NFCController instance.
     *
     * @param[in] driver a pointer to a NFCControllerDriver instance
     * @param[in] queue a pointer to the events queue to use
     * @param[in] ndef_buffer a bytes array used to store NDEF messages
     */
    NFCController(NFCControllerDriver *driver, events::EventQueue *queue, const Span<uint8_t> &ndef_buffer);

    /**
     * Initialize the NFC controller
     *
     * This method must be called before any other method call.
     *
     * @return NFC_OK, or an error.
     */
    nfc_err_t initialize();

    /**
     * Set the delegate that will receive events generated by this controller.
     *
     * @param[in] delegate the delegate instance to use
     */
    void set_delegate(Delegate *delegate);

    /**
     * Get the list of RF protocols supported by this controller.
     *
     * @return a bitmask of RF protocols supported by the controller
     */
    nfc_rf_protocols_bitmask_t get_supported_rf_protocols() const;

    /**
     * Set the list of RF protocols to look for during discovery.
     *
     * @param[in] rf_protocols the relevant bitmask
     * @return NFC_OK on success, or
     *  NFC_ERR_UNSUPPORTED if a protocol is not supported by the controller,
     *  NFC_ERR_BUSY if the discovery process is already running
     */
    nfc_err_t configure_rf_protocols(nfc_rf_protocols_bitmask_t rf_protocols);

    /**
     * Start the discovery process using the protocols configured previously.
     *
     * If remote endpoints are connected when this is called, they will be disconnected.
     *
     * @return NFC_OK on success, or
     *  NFC_ERR_BUSY if the discovery process is already running
     */
    nfc_err_t start_discovery();

    /**
     * Cancel/stop a running discovery process.
     *
     * @return NFC_OK
     */
    nfc_err_t cancel_discovery();

private:
    // These two classes need to be friends to access the following transceiver() method
    friend class NFCRemoteEndpoint;
    friend class Type4RemoteInitiator;
    nfc_transceiver_t *transceiver() const;

    void polling_callback(nfc_err_t ret);

    // NFC Stack scheduler
    void scheduler_process(bool hw_interrupt);

    // Callbacks from NFC stack
    static void s_polling_callback(nfc_transceiver_t *pTransceiver, nfc_err_t ret, void *pUserData);

    // Implementation of NFCControllerDriver::Delegate
    virtual void on_hw_interrupt();

    // Triggers when scheduler must be run again
    void on_timeout();

    NFCControllerDriver *_driver;
    events::EventQueue *_queue;
    nfc_transceiver_t *_transceiver;
    nfc_scheduler_t *_scheduler;
    Timer _timer;
    Timeout _timeout;
    Delegate *_delegate;
    bool _discovery_running;
    Span<uint8_t> _ndef_buffer;
};
/** @}*/
} // namespace nfc
} // namespace mbed

#endif