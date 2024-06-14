#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "ux.h"
#include "eos_parse.h"
#include "eos_stream.h"

ux_state_t G_ux;
uint8_t G_io_apdu_buffer[IO_APDU_BUFFER_SIZE];

txProcessingContext_t txProcessingCtx;
txProcessingContent_t txContent;
cx_sha256_t sha256;
cx_sha256_t sha256_arg;

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    UX_INIT();

    initTxContext(&txProcessingCtx, &sha256, &sha256_arg, &txContent, 1);
    uint8_t status = parseTx(&txProcessingCtx, (uint8_t *) Data, Size);

    if (Size > 0) {
        do {
            switch (status) {
                case STREAM_CONFIRM_PROCESSING:
                case STREAM_ACTION_READY:
                case STREAM_PROCESSING:
                case STREAM_FAULT:
                    return 0;
            }

            status = parseTx(&txProcessingCtx, (uint8_t *) Data, Size);

        } while (status != STREAM_FINISHED);
    }
    return 0;
}
