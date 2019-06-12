#ifndef __SECURE_LOG_T__
#define __SECURE_LOG_T__

#include "log_t.h"

#define SECURE_LOG_ENTRY_LENGTH 256+sizeof(digest)

typedef log_name secure_log_name;
typedef log secure_log;
typedef log_io_stream secure_log_io_stream;
typedef uint8_t digest[256];
typedef struct secure_log_entries {
  log_entry the_entry;
  digest the_digest;
} secure_log_entry;
typedef enum { no_integrity, hashchain_sha2_256, hashchain_sha3_256 } integrity;
typedef enum { no_provenance } provenance;
typedef enum { no_confidentiality, all_plaintext, all_ciphertext } confidentiality;
typedef enum { no_authentication, aes_cbc, hmac_sha2_256, rsa_1025 } authentication;
typedef enum { no_access_control, userid } access_control;
typedef enum { no_non_repudiation } non_repudiation;
typedef struct secure_log_security_policies {
  integrity I;
  provenance P;
  confidentiality C;
  authentication A;
  access_control ACL;
  non_repudiation NR;
} secure_log_security_policy;

#endif
