#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "eos_stream.h"
#include "cx.h"

// Sample binary transaction buffer with two actions (noop and transfer)
// This buffer should be a valid serialized transaction matching the JSON structure seen in corpus files.
// For demonstration, this is a simplified placeholder buffer.
// In real tests, this should be replaced with actual serialized transaction bytes.
static const uint8_t sample_tx_buffer[] = {
    0x04, 0x20, // TLV Chain ID tag and length (32 bytes)
    // 32 bytes chain id (dummy values)
    0xcf, 0x05, 0x7b, 0xbf, 0xb7, 0x26, 0x40, 0x47,
    0x1f, 0xd9, 0x10, 0xbc, 0xb6, 0x76, 0x39, 0xc2,
    0x2d, 0xf9, 0xf9, 0x24, 0x70, 0x93, 0x6c, 0xdd,
    0xc1, 0xad, 0xe0, 0xe2, 0xf2, 0xe7, 0xdc, 0x4f,
    0x10, 0x01, // TLV Header Expiration (dummy)
    0x10, 0x02, // TLV Header Ref Block Num (dummy)
    0x10, 0x04, // TLV Header Ref Block Prefix (dummy)
    0x10, 0x00, // TLV Header Max Net Usage Words (0)
    0x10, 0x00, // TLV Header Max CPU Usage ms (0)
    0x10, 0x00, // TLV Header Delay Sec (0)
    0x10, 0x00, // TLV CFA List Size (0)
    0x10, 0x02, // TLV Action List Size (2 actions)
    // Action 0: Account "null.vaulta" (encoded as name_t)
    0x10, 0x0a, // TLV Action Account length (dummy)
    // Account name bytes (dummy)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x04, // TLV Action Name length (dummy)
    // Action name bytes (dummy)
    0x00, 0x00, 0x00, 0x00,
    0x10, 0x01, // TLV Authorization List Size (1)
    // Authorization actor and permission (dummy)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, // TLV Action Data Size (0)
    0x10, 0x00, // TLV Action Data (empty)
    // Action 1: Account "core.vaulta" (encoded as name_t)
    0x10, 0x0a, // TLV Action Account length (dummy)
    // Account name bytes (dummy)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x08, // TLV Action Name length (dummy)
    // Action name bytes (dummy)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x01, // TLV Authorization List Size (1)
    // Authorization actor and permission (dummy)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x10, // TLV Action Data Size (dummy)
    // Action data bytes (dummy)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, // TLV Tx Extension List Size (0)
    0x10, 0x00  // TLV Context Free Data (0)
};

static void test_initTxContext_action_summary(void) {
    txProcessingContext_t context;
    txProcessingContent_t content;
    cx_sha256_t sha256;
    cx_sha256_t dataSha256;

    // Initialize content to zero
    memset(&content, 0, sizeof(content));

    // Save initial state variables for comparison after initTxContext
    txProcessingState_e expected_state = TLV_CHAIN_ID;
    uint8_t *expected_workBuffer = (uint8_t *)sample_tx_buffer;
    uint32_t expected_commandLength = sizeof(sample_tx_buffer);

    // Initialize context with sample buffer
    initTxContext(&context, &sha256, &dataSha256, &content, 1, 1);

    // Set workBuffer and commandLength to sample buffer for parsing
    context.workBuffer = (uint8_t *)sample_tx_buffer;
    context.commandLength = sizeof(sample_tx_buffer);

    // Save copies of state, workBuffer, commandLength before calling preprocessActions
    txProcessingState_e state_before = context.state;
    uint8_t *workBuffer_before = context.workBuffer;
    uint32_t commandLength_before = context.commandLength;

    // Assertions
    // Check that actionCount matches expected (2 actions in sample buffer)
    assert(content.actionCount == 2);

    // Check that state, workBuffer, commandLength are unchanged after initTxContext
    assert(context.state == expected_state);
    assert(context.workBuffer == expected_workBuffer);
    assert(context.commandLength == expected_commandLength);

    // Check action details (contractName, actionName, argumentCount)
    // Since sample buffer is dummy, we cannot check exact names, but we check argumentCount is non-zero
    for (int i = 0; i < content.actionCount; i++) {
        actionSummary_t *action = &content.actions[i];
        // contractName and actionName are name_t (uint64_t), check non-zero
        assert(action->contractName != 0);
        assert(action->actionName != 0);
        assert(action->argumentCount > 0);
    }

    // Check that parsing state, workBuffer, and commandLength are unchanged after initTxContext
    assert(context.state == state_before);
    assert(context.workBuffer == workBuffer_before);
    assert(context.commandLength == commandLength_before);

    printf("test_initTxContext_action_summary passed\n");
}

int main(void) {
    test_initTxContext_action_summary();
    return 0;
}