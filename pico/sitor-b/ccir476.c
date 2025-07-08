
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "ccir476.h"

typedef enum { LETTERS = 0, FIGURES } CCIR476_MODE;
typedef enum { UNSYNC, SYNC, NORMAL, ERROR } CCIR476_STATUS;

typedef struct {
    CCIR476_MODE mode;          // LETTERS of FIGURE
    CCIR476_STATUS status;      // Status,
} ccir476_decoder_struct;

ccir476_decoder_struct ccir476_decoder;

typedef struct {
    uint8_t ccir_hex;
    char ccir_letters, ccir_figures_ita2, ccir_figures_us_tty;
} ccir476_lookup_struct;

const ccir476_lookup_struct ccir476_lookup[29] = {
    {0x47, 'A', '-',    '-'},
    {0x72, 'B', '?',    '?'},
    {0x1D, 'C', ':',    ':'},
    {0x53, 'D', '\0',   '$'},
    {0x56, 'E', '3',    '3'},
    {0x1B, 'F', '\0',   '!'},
    {0x35, 'G', '\0',   '&'},
    {0x69, 'H', '\0',   '#'},
    {0x4D, 'I', '8',    '8'},
    {0x17, 'J', '\7',   '\''}, //IAT2 -> BELL
    {0x1E, 'K', '(',    '('},
    {0x65, 'L', ')',    ')'},
    {0x39, 'M', '.',    '.'},
    {0x59, 'N', ',',    ','},
    {0x71, 'O', '9',    '9'},
    {0x2D, 'P', '0',    '0'},
    {0x2E, 'Q', '1',    '1'},
    {0x55, 'R', '4',    '4'},
    {0x4B, 'S', '\'',   '\7'}, // US_TTY -> BELL
    {0x74, 'T', '5',    '5'},
    {0x4E, 'U', '7',    '7'},
    {0x3C, 'V', '=',    ';'},
    {0x27, 'W', '2',    '2'},
    {0x3A, 'X', '/',    '/'},
    {0x2B, 'Y', '6',    '6'},
    {0x63, 'Z', '+',    '"'},
    {0x78, '\r','\r',   '\r'}, // CR
    {0x6C, '\n','\n',   '\n'}, // LF
    {0x5C, ' ', ' ',    ' '}   // SPACE
};

// local functions prototype
void sitorb_fsm_reset(void);
//uint8_t ccir476_decode(uint8_t byte);
bool ccir476_decode(uint8_t byte, uint8_t *c);


// Local vars ccir476 bit processor
uint8_t ccir476_bitbuf = 0;
uint8_t ccir476_bitcnt = 0;
bool ccir476_sync = false;
uint8_t ccir476_sync_cnt = 0;

//fsm callbacks
static void (*sitorb_msg_available) (char *) = NULL;
static void (*sitorb_char_available) (char ch) = NULL;
static void (*sitorb_err) (char *) = NULL;

// Local vars for the SITOR-B fsm
typedef enum {SB_S0, SB_S1, SB_S2, SB_S3, SB_SERR } SITORB_STATE_ENUM;
char sitorb_msg[2048];
uint8_t sitorb_msg_index = 0;
SITORB_STATE_ENUM sitorb_state = SB_S0;
uint8_t sitorb_fsm_cycle_cnt = 0;


/*******************************************************************/
bool ccir476_decode(uint8_t byte, uint8_t *c)
/* 
short:         Decode ccir476 byte to us_tty of ita2. 
inputs:        
outputs: 
notes:         Keeps track of the LETTERS/FIGURES state.
Version :      DMK, Initial code
********************************************************************/
{
    bool retval = true;

    switch (byte) {
        case 0x0F: // MODE-B (FEC) -> Phasing signal 1 idle signal alpha
            //printf("[%c]\n", 0x0F);
            *c = 0x0F ; 
            break;    
        
        case 0x33: // MODE-B (FEC) -> Idle signal Beta
            //printf("[%c]\n", 0x33);
            *c = 0x33;
            break;    
        
        case 0x66:  // MODE-B (FEC) -> Phasing signal 2
            //printf("[%c]\n",0x66);
            *c = 0x66; 
            break;
        
        case 0x5A: // Change character set to LETTERS
            //printf("[%c]", 0x5a);
            ccir476_decoder.mode = LETTERS;
            *c = 0x5A; // '\0';
            break;
        
        case 0x36: // Change character set to FIGURES
            //printf("[%c]", 0x36);
            ccir476_decoder.mode = FIGURES;
            *c = 0x36; // '\0';
            break;

        case 0x6A: 
            //printf("[%c]", 0x6A);
            *c = 0x6A;
            break;
        
        default: {
            bool found = false;
            for (uint8_t idx = 0; idx < 29; idx++) {
                if (byte == ccir476_lookup[idx].ccir_hex) {
                    if (ccir476_decoder.mode == LETTERS) {
                        *c = ccir476_lookup[idx].ccir_letters;
                    }
                    if (ccir476_decoder.mode == FIGURES) {
                        *c = ccir476_lookup[idx].ccir_figures_us_tty;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                *c = 0x00;
                retval = false;
            }
            break;
        }
    }

    return retval;
}

/*******************************************************************/
void ccir476_init( 
    void (*msg)(char *),     
    void (*c)(char ch), 
    void (err)(char *) ) 
/* 
short:         Rearm the ccir476 bitprocessor and statemachine
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
********************************************************************/
{
    // set msg callback
    sitorb_msg_available = msg;
    sitorb_char_available = c;
    sitorb_err = err;

    // and rearm
    ccir476_rearm();
}

/*******************************************************************/
void ccir476_rearm() 
/* 
short:         Rearm the ccir476 bitprocessor and statemachine
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
********************************************************************/
{
    // Reset bit processor
    ccir476_bitbuf = 0;
    ccir476_bitcnt = 0;
    ccir476_sync = false;
    ccir476_sync_cnt = 0;

    // reset sitorb fsm
    sitorb_fsm_reset();
}


/* 
 * SITOR-B simple statemachine. Return SITOR-B message via callback
 * 
 */

/*******************************************************************/
void sitorb_fsm_reset()
/* 
short:      Reset sitor_b fsm
inputs:        
outputs: 
notes:         
Version:    DMK, Initial code
********************************************************************/
{
    sitorb_state = SB_S0;
    sitorb_fsm_cycle_cnt = 0;
    sitorb_msg_index = 0;
}

/*******************************************************************/
void sitorb_fsm(uint8_t byte) 
/* 
short:      Simple sitor_b fsm 
inputs:        
outputs: 
notes:         
Version:    DMK, Initial code
********************************************************************/
{
    switch (sitorb_state) {
        
        // Handle phasing-1 and phasing-2: 0x0F ... 0x0F ... 0x66 0xFF ...
        case SB_S0:
            if (0x0F == byte) { 
                sitorb_state = SB_S0; 
            } else
            if (0x66 == byte) { 
                sitorb_state = SB_S1; 
            } else {
                sitorb_state = SB_S2;
                sitorb_fsm_cycle_cnt = 0;
                uint8_t ch;
                if( true == ccir476_decode(byte, &ch) ) {
                    // Character available, filter LETTERS/FIGURES
                    if(sitorb_fsm_cycle_cnt % 2 == 0x00 && 0x5A != byte && 0x36 != byte) {
                        sitorb_msg[sitorb_msg_index++] = (char)ch;
                        //printf("%c", (char) ch);
                        // character cb here
                        if( sitorb_char_available != NULL ) {
                            sitorb_char_available((char)ch); 
                        }
                    }
                } else {
                    sitorb_state = SB_SERR;
                }
            }
            break;                      

        case SB_S1:
            if (byte == 0x0F) 
                sitorb_state = SB_S0;
            else 
                sitorb_state = SB_SERR;
            break;

        // SITOR-B message comming in ... store all character
        case SB_S2:
            if (byte == 0x66) {
                sitorb_state = SB_S3;
                sitorb_msg[sitorb_msg_index] = '\0'; // NULL terminated sitorb msg,
                // complete navtex message cb here
                if( sitorb_msg_available != NULL ) {
                    sitorb_msg_available(sitorb_msg); 
                }
            } else {
                sitorb_state = SB_S2;
                uint8_t ch;
                if( true == ccir476_decode(byte, &ch) ) {
                    // Character available, filter LETTERS/FIGURES. 
                    if (sitorb_fsm_cycle_cnt % 2 == 0 && (0x5A != byte && 0x36 != byte)) {
                        sitorb_msg[sitorb_msg_index++] = (char)ch;
                        if( sitorb_char_available != NULL ) {
                            sitorb_char_available((char)ch); 
                        }
                    }
                } else {
                    sitorb_state = SB_SERR;
                }
            }
            break;
        
        // End of sitor-b message - wait for fsm-reset
        case SB_S3:
            sitorb_state = SB_S3;
            break;

        // Something went wrong
        case SB_SERR:
            if( sitorb_err != NULL ) {
                sitorb_err("sitorb fsm err"); 
            }
            sitorb_state = SB_S3;
            break;
    }

    sitorb_fsm_cycle_cnt++;
}


/*******************************************************************/
void ccir476_process_bit(bool bit) 
/* 
short:      ccir476 bit processor. Sync to 0x0F for at least 6 times 
inputs:        
outputs: 
notes:         
Version:    DMK, Initial code
********************************************************************/
{
    uint8_t value = (bit & 0x01) << (7-1);
    ccir476_bitbuf = (ccir476_bitbuf >> 1) | value;

    if(!ccir476_sync) {
        if( ccir476_bitbuf == 0x0F ) {
            if(ccir476_sync_cnt == 6) {
                ccir476_sync = true;    // Were in sync, reset fsm
                sitorb_fsm_reset();
                printf("[Sync]\n");
            }
            ccir476_sync_cnt++;
            ccir476_bitbuf = 0;
            ccir476_bitcnt = 0;
        }
    } else {
        // Sync = true -> collect 7 bits
        ccir476_bitcnt++;
        if(ccir476_bitcnt == 7) {
            // 7 bits ccir476 captured. Pass to fsm
            // and get ready for next 7 bits
            // to do: check byte (exactly 4 '1''s)
            sitorb_fsm(ccir476_bitbuf);
            ccir476_bitcnt = 0;            
            ccir476_bitbuf = 0;
        }
    }
}
 
