/**
 * Smart Ballot Box API
 * @refine sbb.lando
 */
#ifndef __SBB_H__
#define __SBB_H__

// General includes
#include <stdbool.h>
#include <stdint.h>

// Subsystem includes
#include "sbb_t.h"
#include "sbb.acsl"

// Display strings
extern const char *insert_ballot_text;
extern const char *barcode_detected_text;
extern const char *cast_or_spoil_text;
extern const char *casting_ballot_text;
extern const char *spoiling_ballot_text;
extern const char *not_a_valid_barcode_text;
extern const char *no_barcode_text;
extern const char *remove_ballot_text;

extern SBB_state the_state;

// @todo kiniry This is a placeholder state representation so that we
// can talk about the state of memory-mapped firmware.  It should
// probably be refined to a separate memory-mapped region (or more
// than one) per device and an invariant should stipulate that the
// memory regions are distinct.
typedef bool firmware_state;
extern firmware_state the_firmware_state;

// @spec abakst Specifications needed for for hardware, related to the
// above I think we probably want some ghost `uint8_t` array to model
// these reads/writes.
extern uint8_t gpio_mem[8];

/* Button defines */
#define BUTTON_CAST_LED 3
#define BUTTON_SPOIL_LED 1
#define BUTTON_CAST_IN 2
#define BUTTON_SPOIL_IN 0

/* Paper sensor inputs */
#define PAPER_SENSOR_OUT 6
#define PAPER_SENSOR_IN 7

// Motor defines
#define MOTOR_0 4
#define MOTOR_1 5

/*@ assigns \nothing; */
uint8_t gpio_read(uint8_t i);

/*@ assigns gpio_mem[i]; */
void    gpio_write(uint8_t i);

/*@ assigns gpio_mem[i]; */
void    gpio_clear(uint8_t i);


// @design kiniry I am presuming that this function must be called
// prior to any other function and guarantees that all devices are put
// in their initial state.

// @design kiniry We could (should?) encode device driver/hardware
// subsystem state explicitly and state a separated clause between
// them here, something like
//   \separated(sd_card_dd_state, time_dd_state, etc.);

// @design kiniry All of these functions that are commands have a
// tight frame axiom, as they state exactly what part of `the_state`
// of the system is update, which is the model state for the SBB ASM.
// They must also explicitly state which mem-mapped state they modify
// in the implementation itself for compositional reasoning to be
// sound.

/*@ assigns the_firmware_state;
*/
// @todo Should immediately transition to `go_to_standby()`.
// @assurance kiniry The implementation of `initialize` must have a
// the C label `DevicesInitialized` on its final statement.
void initialize(void);

// @review kiniry Needs a postcondition that states that the currently
// held ballot is a legal ballot for the election, as soon as the crypto
// spec is ready for use.
/*@ requires \valid(the_barcode + (0 .. its_length));
  @ requires the_state.P == EARLY_AND_LATE_DETECTED;
  @ assigns \nothing;
*/
bool is_barcode_valid(barcode_t the_barcode, barcode_length_t its_length);

/*@ assigns \nothing;
  @ ensures \result == (the_state.B == CAST_BUTTON_DOWN);
*/
bool is_cast_button_pressed(void);

/*@ assigns \nothing;
  @ ensures \result == (the_state.B == SPOIL_BUTTON_DOWN);
*/
bool is_spoil_button_pressed(void);

/*@ requires the_state.P == EARLY_AND_LATE_DETECTED;
  @ assigns \nothing;
  @ ensures \result == (the_state.BS == BARCODE_PRESENT_AND_RECORDED);
*/
bool has_a_barcode(void);

// @review kiniry Is the intention that this set of functions
// (just_received_barcode, set_received_barcode, and
// what_is_the_barcode) is the underlying (non-public) functions
// encoding the device driver/firmware for the barcode scanner
// subsystem?
/*@ requires \valid(the_barcode + (0 .. its_length));
  @ requires the_state.BS == BARCODE_PRESENT_AND_RECORDED;
*/
// assigns the_barcode + (0 .. its_length);
// ensures (* the model barcode is written to the_barcode *)
// @design kiniry Should this function return the number of bytes in
// the resulting barcode?
void what_is_the_barcode(barcode_t the_barcode, barcode_length_t its_length);

// @review kiniry We have no ASM for button light states.
/*@ assigns the_state.button_illumination;
  @ ensures spoil_button_lit(the_state);
*/
void spoil_button_light_on(void);

/*@ assigns the_state.button_illumination;
  @ ensures !spoil_button_lit(the_state);
*/
void spoil_button_light_off(void);

/*@ assigns the_state.button_illumination;
  @ ensures cast_button_lit(the_state);
*/
void cast_button_light_on(void);

/*@ assigns the_state.button_illumination;
  @ ensures !cast_button_lit(the_state);
*/
void cast_button_light_off(void);

/*@ requires the_state.M == MOTORS_TURNING_FORWARD ||
  @          the_state.M == MOTORS_OFF;
  @ assigns the_state.M,
  @         gpio_mem[MOTOR_0],
  @         gpio_mem[MOTOR_1];
  @ ensures the_state.M == MOTORS_TURNING_FORWARD;
  @ ensures ASM_transition(\old(the_state), MOTOR_FORWARD_E, the_state);
*/
void move_motor_forward(void);

/*@ requires the_state.M == MOTORS_TURNING_BACKWARD ||
  @          the_state.M == MOTORS_OFF;
  @ assigns the_state.M,
  @         gpio_mem[MOTOR_0],
  @         gpio_mem[MOTOR_1];
  @ ensures the_state.M == MOTORS_TURNING_BACKWARD;
  @ ensures ASM_transition(\old(the_state), MOTOR_BACKWARD_E, the_state);
  @*/
void move_motor_back(void);

/*@ assigns the_state.M,
            gpio_mem[MOTOR_0],
            gpio_mem[MOTOR_1];
  @ ensures the_state.M == MOTORS_OFF;
  @ ensures ASM_transition(\old(the_state), MOTOR_OFF_E, the_state);
*/
void stop_motor(void);

// @design kiniry What is the memory map for the display?  We should
// be able to specify, both at the model and code level, what is on
// the display.

/*@ requires \valid_read(str + (0 .. len));
  @ assigns the_state.D;
  @ ensures the_state.D == SHOWING_TEXT;
  @ ensures ASM_transition(\old(the_state), DISPLAY_TEXT_E, the_state);
*/
void display_this_text(const char* str, uint8_t len);

/*@ assigns \nothing;
  @ ensures \result == (the_state.P == EARLY_PAPER_DETECTED);
*/
bool ballot_detected(void);

/*@ requires (the_state.P == EARLY_PAPER_DETECTED);
  @ assigns \nothing;
  @ ensures \result == (the_state.P == EARLY_AND_LATE_DETECTED);
*/
bool ballot_inserted(void);

/*@ requires spoil_button_lit(the_state);
  @ requires the_state.P == EARLY_AND_LATE_DETECTED;
  @ assigns the_state.button_illumination,
  @         the_state.P;
  @ ensures no_buttons_lit(the_state);
  @ ensures the_state.P == NO_PAPER_DETECTED;
  @ ensures ASM_transition(\old(the_state), SPOIL_E, the_state);
*/
void spoil_ballot(void);

/*@ requires cast_button_lit(the_state);
  @ requires the_state.P == EARLY_AND_LATE_DETECTED;
  @ assigns the_state.button_illumination,
  @         the_state.P;
  @ ensures no_buttons_lit(the_state);
  @ ensures the_state.P == NO_PAPER_DETECTED;
  @ ensures ASM_transition(\old(the_state), CAST_E, the_state);
*/
void cast_ballot(void);

/*@ assigns the_state.P;
  @ ensures \result == the_state.P == EARLY_PAPER_DETECTED;
*/
bool ballot_spoiled(void);

// Semi-equivalent to initialize() without firmware initialization.
// @review Shouldn't calling this function clear the paper path?
/*@ assigns the_state.M, the_state.D, the_state.P, the_state.BS, the_state.S;
  @ ensures the_state.M == MOTORS_OFF;
  @ ensures the_state.D == SHOWING_TEXT;
  @ ensures the_state.P == NO_PAPER_DETECTED;
  @ ensures the_state.BS == BARCODE_NOT_PRESENT;
  @ ensures the_state.S == INNER;
  @ ensures no_buttons_lit(the_state);
*/
// @todo kiniry `insert_ballot_text` should be displayed.
void go_to_standby(void);

// @design kiniry These next four functions should also probably move
// out of this API, right Michal?

//@ assigns \nothing;
void ballot_detect_timeout_reset(void);

//@ assigns \nothing;
bool ballot_detect_timeout_expired(void);

//@ assigns \nothing;
void cast_or_spoil_timeout_reset(void);

//@ assigns \nothing;
bool cast_or_spoil_timeout_expired(void);

/*@ terminates \false;
    ensures \false;
*/
void ballot_box_main_loop(void);

#endif /* __SBB_H__ */
