/**
 * Smart Ballot Box Logging Implementation
 */
#include "sbb_logging.h"

const log_name system_log_file_name = "system_log.txt";
const log_name app_log_file_name    = "application_log.txt";

Log_Handle app_log_handle;
Log_Handle system_log_handle;

#ifndef FILESYSTEM_DUPLICATES
uint8_t scanned_codes[1000][BARCODE_MAX_LENGTH];
uint16_t num_scanned_codes = 0;
#endif // FILESYSTEM_DUPLICATES

// Each entry should be a 0-padded 256 uint8_t array according to the c specification.
const uint8_t app_event_entries[] = { 'C', 'S' };

bool import_and_verify(log_file the_file) {
    #ifdef SIMULATION
    return false;
    #else
    bool b_success = false;

    if ( import_log(the_file) ) {
        b_success = verify_log_well_formedness(the_file);
    }

    return b_success;
    #endif
}

bool load_or_create(log_file the_file,
                    const log_name the_name,
                    const http_endpoint endpoint) {
    #ifdef SIMULATION
    return false;
    #else

    // @todo There is no API for opening a file for write, so we will overwrite for now
    bool b_success = true;

    if ( Log_IO_File_Exists(the_name) &&
         LOG_FS_OK == Log_IO_Open(the_file, the_name) ) {
        b_success = import_and_verify(the_file);
    } else if ( LOG_FS_OK == create_log(the_file, the_name, endpoint) ) {
        b_success = true;
    } else {
        b_success = false;
    }

    return b_success;
    #endif
}

bool load_or_create_logs(void) {
    #ifdef SIMULATION
    return true;
    #else
    bool b_success = false;

    if (load_or_create(&app_log_handle,
                       app_log_file_name,
                       HTTP_Endpoint_App_Log))
      {
        if (load_or_create(&system_log_handle,
                           system_log_file_name,
                           HTTP_Endpoint_Sys_Log))
          {
            b_success = true;
          }
      }

    return b_success;
    #endif
}

bool log_system_message(const log_entry new_entry) {
    #ifdef SIMULATION
    debug_printf("LOG: %s\r\n", new_entry);
    return true;
    #else
    Log_FS_Result res = write_entry(&system_log_handle, new_entry);
    return (res == LOG_FS_OK);
    #endif
}

void log_or_abort(SBB_state *the_state, const log_entry the_entry) {
    debug_printf((char *)the_entry);
    #ifdef SIMULATION
    debug_printf("LOG: %s\r\n", the_entry);
    #else
    if (!log_system_message(the_entry)) {
        the_state->L = ABORT;
    }
    #endif
}

// @design abakst I think this is going to change as the logging implementation is fleshed out
// For example, we should be logging time stamps as well.
bool log_app_event(app_event event,
                   barcode_t barcode,
                   barcode_length_t barcode_length) {
    if ( barcode_length + 2 < LOG_ENTRY_LENGTH ) {
        log_entry event_entry;
        memset(&event_entry, 0x20, sizeof(log_entry)); // pad with spaces
        event_entry[0] = app_event_entries[event];
        // we're guaranteed there are no spaces in the Base64 barcode, so it runs from [2] to
        // the next space in the entry
        memcpy(&event_entry[2], barcode, barcode_length);
#ifndef FILESYSTEM_DUPLICATES
        for (size_t i = 0; i < barcode_length; i++) {
            scanned_codes[num_scanned_codes][i] = (uint8_t)barcode[i];
        }
        for (size_t i = barcode_length; i < 256; i++) {
            scanned_codes[num_scanned_codes][i] = 0;
        }
        num_scanned_codes = num_scanned_codes + 1;
#endif
#ifdef SIMULATION
        debug_printf("LOG: %c %hhu", (char)app_event_entries[event], (uint8_t)barcode_length);
        for (size_t i = 0; i < barcode_length; i++) {
            debug_printf(" %hhx", (uint8_t)barcode[i]);
        }
        debug_printf("\r\n");
        return true;
#else
        Log_FS_Result res = write_entry(&app_log_handle, event_entry);
        return (res == LOG_FS_OK);
#endif
    } else {
        return false;
    }
}

bool barcode_cast_or_spoiled(barcode_t barcode, barcode_length_t length) {
    bool b_found = false;
#ifdef FILESYSTEM_DUPLICATES
    size_t n_entries = Log_IO_Num_Base64_Entries(&app_log_handle);

    if (length >= BARCODE_MAX_LENGTH) {
        debug_printf("barcode is too long, treated as duplicate");
        b_found = true; // treat too-long barcode as duplicate
    } else {
        debug_printf("scanning for duplicates, there are %d entries", n_entries);
        /** Scan the log backwards. The 0th entry is not a real entry to consider. */
        // note int32_t below because size_t is unsigned and subtraction 1 from it
        // yields a large positive number - hat tip to Haneef
        for (int32_t i_entry_no = n_entries - 1; !b_found && (i_entry_no > 1); i_entry_no--) {
            debug_printf("scanning entry %d", i_entry_no);
            secure_log_entry secure_entry = Log_IO_Read_Base64_Entry(&app_log_handle, i_entry_no);
            b_found = true;
            // compare the barcodes up to the new barcode's length
            for (size_t i_barcode_idx = 0;
                 b_found && (i_barcode_idx < length) && (i_barcode_idx < BARCODE_MAX_LENGTH);
                 i_barcode_idx++) {
                b_found &= secure_entry.the_entry[2 + i_barcode_idx] == barcode[i_barcode_idx];
            }
            // ensure that the next character, if any, in the previously recorded barcode
            // is a space
            if (length + 1 < BARCODE_MAX_LENGTH) { // we haven't already checked the full width
                b_found &= secure_entry.the_entry[2 + length] == 0x20;
            }
        }
    }
#else // FILESYSTEM_DUPLICATES
    if (length >= BARCODE_MAX_LENGTH) {
        debug_printf("barcode is too long, treated as duplicate");
        b_found = true; // treat too-long barcode as duplicate
    } else {
        debug_printf("scanning for duplicates, there are %d entries", num_scanned_codes);
        for (uint16_t i_entry_no = 0; !b_found && (i_entry_no < num_scanned_codes); i_entry_no++) {
            debug_printf("scanning entry %d", i_entry_no);
            b_found = true;
            for (size_t i_barcode_idx = 0; b_found && (i_barcode_idx < length); i_barcode_idx++) {
                b_found &= scanned_codes[i_entry_no][i_barcode_idx] == barcode[i_barcode_idx];
            }
            // ensure that the next character, if any, in the previously recorded barcode
            // is a space
            if (length + 1 < BARCODE_MAX_LENGTH) { // we haven't already checked the full width
                b_found &= scanned_codes[i_entry_no][length] == 0x20;
            }
        }
        debug_printf("barcode is a duplicate: %d", b_found);
    }
    return b_found;
#endif // FILESYSTEM_DUPLICATES
}
