#include <stdlib.h>
#include <nfc/nfc.h>
#include <string.h>
#include <stdio.h>

// STRASBOURG CTS DECODER
// Target: ST SRI512 (ISO14443-B)

// Buffer to store the ticket memory for analysis
uint8_t ticket_memory[64][4]; 

void print_hex(const uint8_t *pbtData, const size_t szBytes) {
  for (size_t szPos = 0; szPos < szBytes; szPos++) {
    printf("%02x ", pbtData[szPos]);
  }
  printf("  ");
  for (size_t szPos = 0; szPos < szBytes; szPos++) {
    if (pbtData[szPos] >= 32 && pbtData[szPos] <= 126)
      printf("%c", pbtData[szPos]);
    else
      printf(".");
  }
  printf("\n");
}

void analyze_ticket() {
    printf("\n" "========================================\n");
    printf("       TICKET FORENSICS REPORT          \n");
    printf("========================================\n");

    // --- ANALYZING BLOCK 10 (PRODUCT TYPE) ---
    uint8_t product_byte = ticket_memory[10][1];
    printf("[*] TICKET TYPE:    ");
    if (product_byte == 0x24) {
        printf("24 HOUR PASS (Detected)\n");
    } else {
        printf("Unknown (Hex ID: %02X)\n", product_byte);
    }

    // --- ANALYZING BLOCK 05 (COUNTER) ---
    uint8_t counter_val = ticket_memory[5][1];
    printf("[*] COUNTER VALUE:  %d (Raw Hex: %02X)\n", counter_val, counter_val);
    if (counter_val == 10) printf("    (Likely initialized as a 10-count structure)\n");

    // --- ANALYZING BLOCK 13 (VALIDATION TIME) ---
    // Byte 0 (6a) = Time units (5 min intervals from 4:00 AM)
    // Byte 1 (16) = Day of Month
    
    uint8_t time_byte = ticket_memory[13][0];
    uint8_t day_byte  = ticket_memory[13][1];
    
    // CTS Logic: Time is usually (Value * 5) minutes past 04:00 AM
    int total_minutes = time_byte * 5;
    int start_hour_offset = 4; // Service day starts at 4am
    
    int hours_added = total_minutes / 60;
    int minutes_rem = total_minutes % 60;
    
    int final_hour = start_hour_offset + hours_added;
    
    printf("[*] LAST RIDE:      Day %d of the month\n", day_byte);
    printf("[*] TIME STAMP:     %02d:%02d (Calculated)\n", final_hour, minutes_rem);
    
    printf("========================================\n\n");
}

int main(int argc, const char *argv[]) {
  nfc_device *pnd;
  nfc_target nt;
  nfc_context *context;

  // Initialize memory buffer with 0
  memset(ticket_memory, 0, sizeof(ticket_memory));

  nfc_init(&context);
  if (context == NULL) exit(EXIT_FAILURE);

  pnd = nfc_open(context, NULL);
  if (pnd == NULL) {
    printf("ERROR: Unable to open NFC device. (Stop pcscd!)\n");
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    exit(EXIT_FAILURE);
  }

  printf("--- SEARCHING FOR CTS TICKET ---\n");
  printf("[*] Place ticket on the RIM/EDGE now...\n");

  nfc_modulation nm;
  nm.nmt = NMT_ISO14443B2SR; // Specific ST SRx modulation
  nm.nbr = NBR_106;

  // Poll
  while (nfc_initiator_select_passive_target(pnd, nm, NULL, 0, &nt) <= 0) {}

  printf("\n[+] TICKET FOUND! (UID: ");
  print_hex(nt.nti.nsi.abtUID, 8); 
  printf(")\n\n");

  // Read loop
  uint8_t cmd[2];
  uint8_t res[255];
  int res_len;

  printf("Reading memory...\n");
  for (int i = 0; i < 16; i++) { // Read first 16 blocks (SRI512 size)
      cmd[0] = 0x08; 
      cmd[1] = (uint8_t)i;

      res_len = nfc_initiator_transceive_bytes(pnd, cmd, 2, res, sizeof(res), 0);

      if (res_len > 0) {
          // Store in our memory buffer for analysis
          // Note: res includes a status byte usually, but ST SRx raw read is often pure data
          // We assume 'res' holds the 4 bytes of the block directly here.
          memcpy(ticket_memory[i], res, 4);
          
          printf("Blk %02d | ", i);
          print_hex(res, res_len);
      }
  }
  
  // Trigger the analysis
  analyze_ticket();

  nfc_close(pnd);
  nfc_exit(context);
  return 0;
}
