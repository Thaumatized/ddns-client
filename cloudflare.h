#pragma once

#include "c-jsonc/json.h"
#include "ipUtils.h"

/**
 * Initializes the cloudflare module.
 * - Verification of config.
 * - Fetching Zone and Record IDs
 * 
 * returns byteflagged int for required address protocols.
 */
int setCloudflareConfigs(JSON *configs);

/**
 * Updates the records.
 */
void updateCloudflareRecords(IpAddresses addresses, int updated);