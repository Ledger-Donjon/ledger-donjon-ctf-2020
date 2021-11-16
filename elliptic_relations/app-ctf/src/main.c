#include "os.h"
#include "cx.h"
#include "ux.h"
#include "os_io_seproxyhal.h"
#include "glyphs.h"

#include "challenge.h"

#include <stdbool.h>
#include <string.h>

enum {
  SW_CLA_NOT_SUPPORTED = 0x6e00,
  SW_WRONG_LENGTH = 0x6700,
  SW_DATA_INVALID = 0x6984,
  SW_NO_ERROR = 0x9000,
};

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define CLA 0xE0
#define INS_RESET 0x01
#define INS_FILL  0x02
#define INS_CHECK 0x03

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

void ui_idle(void);

UX_STEP_NOCB(
    ux_idle_flow_1_step,
    pnn,
    {
        &C_icon_donjon,
        "Can you find",
        "my secret?",
    });
UX_STEP_NOCB(
    ux_idle_flow_2_step,
    bn,
    {
        "Version",
        APPVERSION,
    });
UX_STEP_VALID(
    ux_idle_flow_3_step,
    pb,
    os_sched_exit(0),
    {
        &C_icon_dashboard,
        "Quit app",
    });
UX_FLOW(ux_idle_flow,
    &ux_idle_flow_1_step,
    &ux_idle_flow_2_step,
    &ux_idle_flow_3_step,
    FLOW_END_STEP
);

static void send_response(uint8_t tx, bool approve) {
    G_io_apdu_buffer[tx++] = approve? 0x90 : 0x69;
    G_io_apdu_buffer[tx++] = approve? 0x00 : 0x85;
    // Send back the response, do not restart the event loop

    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
}

static const uint8_t encoded_flag[26] = {
	0xC9, 0x66, 0x4B, 0x7A, 0x20, 0x34, 0xE3, 0x99, 0x36, 0x23, 0x42, 0x28,
	0x1F, 0xE2, 0x7C, 0x5C, 0xCC, 0x5C, 0xF2, 0x95, 0x52, 0x18, 0xDF, 0xE8,
	0x92, 0x93
};

UX_STEP_VALID(
  ux_success_step,
  pb,
  {
    cx_sha256_t hash_ctx; \
    cx_sha256_init(&hash_ctx); \
    cx_hash((cx_hash_t *) &hash_ctx, CX_LAST, (uint8_t *) array, GRID_SIZE, G_io_apdu_buffer, CX_SHA256_SIZE); \
    for (size_t i = 0; i < sizeof(encoded_flag); i++) { \
      G_io_apdu_buffer[i] ^= encoded_flag[i]; \
    } \
    send_response(26, true);
  }, {
    &C_icon_validate_14,
    "Congrats!"
  });

UX_FLOW(ux_success,
    &ux_success_step,
    FLOW_END_STEP
);

void ui_idle(void) {
  // reserve a display stack slot if none yet
  if(G_ux.stack_count == 0) {
    ux_stack_push();
  }
  ux_flow_init(0, ux_idle_flow, NULL);
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

static int handle_fill_apdu(const uint8_t *data, size_t len) {
  int err;

  if (len & 1) {
    return -1;
  }
  for (size_t i = 0; i < len; i += 2) {
    err = fill_grid_entry(data[i], data[i + 1]);
    if (err) {
      return err;
    }
  }
  return 0;
}

int handle_apdu(volatile unsigned int *flags, size_t rx, volatile size_t *tx) {
  *tx = 0;
  // if the buffer doesn't start with the magic byte, return an error.
  if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
    THROW(SW_CLA_NOT_SUPPORTED);
  }
  if (rx < OFFSET_CDATA || (rx != G_io_apdu_buffer[OFFSET_LC] + OFFSET_CDATA)) {
    THROW(SW_WRONG_LENGTH);
  }

  // check the second byte (0x01) for the instruction.
  switch (G_io_apdu_buffer[OFFSET_INS]) {

  case INS_RESET:
    reset_grid();
    *tx = 0;
    THROW(SW_NO_ERROR);
    break;

  case INS_FILL: {
    size_t data_size = G_io_apdu_buffer[OFFSET_LC];
    uint8_t *data_ptr = G_io_apdu_buffer + OFFSET_CDATA;
    *tx = 0;

    if (handle_fill_apdu(data_ptr, data_size)) {
      THROW(SW_DATA_INVALID);
    } else {
      THROW(SW_NO_ERROR);
    }
    break;    
  }
  case INS_CHECK: {
    *tx = 0;

    if (!check_grid(array)) {
      reset_grid();
      send_response(0, false);
    } else {
      *flags = IO_ASYNCH_REPLY;
      ux_flow_init(0, ux_success, NULL);
    }
  }
  break;

  default:THROW(0x6D00);
  }
  return 0;
}

void ctf_main(void) {
    size_t rx = 0, tx = 0;
    unsigned int flags = 0;

    reset_grid();

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                handle_apdu(&flags, rx, &tx);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    // return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
            !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
              SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
            THROW(EXCEPTION_IO_RESET);
        }
    // no break is intentional
    default:
        UX_DEFAULT_EVENT();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                USB_power(1);

                ui_idle();
                ctf_main();
            }
                CATCH(EXCEPTION_IO_RESET) {
                    // reset IO and UX
                    continue;
                }
                CATCH_ALL {
                    break;
                }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();

    return 0;
}
