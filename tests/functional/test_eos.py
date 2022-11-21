import json
from pathlib import Path

from ragger.backend import SpeculosBackend
from ragger.backend.interface import RAPDU, RaisePolicy
from ragger.navigator import NavInsID, NavIns
from ragger.utils import pack_derivation_path, split_message

from apps.eos import EosClient, ErrorType, MAX_CHUNK_SIZE
from apps.eos_transaction_builder import Transaction


TESTS_ROOT_DIR = Path(__file__).parent

# Proposed EOS derivation paths for tests ###
EOS_PATH = pack_derivation_path("m/44'/194'/12345'")

SPECULOS_EXPECTED_PUBLIC_KEY = "04a478ace4ac9cdbc8ccfe5560940a2c"\
                               "cdc96d4f7789e7dd4074dbe1eb7865b0"\
                               "0889833972fdafcbd25e71f7515c27c1"\
                               "23449309873e0d16fea13abd2697c035ef"

SPECULOS_EXPECTED_ADDRESS = "EOS85fjM4VLKEYZwJE5FBUhXR3HaFno1t7fpukBfzjm9xUHgzLpuV"

SPECULOS_EXPECTED_CHAINCODE = "007c54db71630a77129b2183b701a6da"\
                              "1cde07a1f4edb1d8ee2f51a14306b4c5"


def get_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.RIGHT_CLICK)] * num_screen_skip
    instructions.append(NavIns(NavInsID.BOTH_CLICK))
    return instructions


def test_eos_mainmenu_and_setting(client, test_name, navigator):
    eos = EosClient(client)

    # Get appversion and "data_allowed parameter"
    data_allowed, version = eos.send_get_app_configuration()
    assert data_allowed is False
    assert version == (1, 4, 4)

    # Navigate in the main menu and the setting menu
    # Change the "data_allowed parameter" value
    instructions = [
        NavIns(NavInsID.RIGHT_CLICK),
        NavIns(NavInsID.RIGHT_CLICK),
        NavIns(NavInsID.RIGHT_CLICK),
        NavIns(NavInsID.LEFT_CLICK),
        NavIns(NavInsID.BOTH_CLICK),
        NavIns(NavInsID.BOTH_CLICK),
        NavIns(NavInsID.RIGHT_CLICK),
        NavIns(NavInsID.BOTH_CLICK)
    ]
    navigator.navigate_and_compare(TESTS_ROOT_DIR, test_name, instructions)

    # Check that "data_allowed parameter" changed
    data_allowed, version = eos.send_get_app_configuration()
    assert data_allowed is True
    assert version == (1, 4, 4)


def check_get_public_key_resp(client, public_key, address, chaincode):
    if isinstance(client, SpeculosBackend):
        # Check against nominal Speculos seed expected results
        assert public_key.hex() == SPECULOS_EXPECTED_PUBLIC_KEY
        assert address == SPECULOS_EXPECTED_ADDRESS
        assert chaincode.hex() == SPECULOS_EXPECTED_CHAINCODE


def test_eos_get_public_key_non_confirm(client):
    eos = EosClient(client)

    rapdu: RAPDU = eos.send_get_public_key_non_confirm(EOS_PATH, True)
    public_key, address, chaincode = eos.parse_get_public_key_response(rapdu.data, True)
    check_get_public_key_resp(client, public_key, address, chaincode)

    # Check that with NO_CHAINCODE, value stay the same
    rapdu: RAPDU = eos.send_get_public_key_non_confirm(EOS_PATH, False)
    public_key_2, address_2, chaincode_2 = eos.parse_get_public_key_response(rapdu.data, False)
    assert public_key_2 == public_key
    assert address_2 == address
    assert chaincode_2 is None


def test_eos_get_public_key_confirm(test_name, client, firmware, navigator):
    eos = EosClient(client)
    if firmware.device == "nanos":
        instructions = get_review_instructions(5)
    else:
        instructions = get_review_instructions(3)
    with eos.send_async_get_public_key_confirm(EOS_PATH, True):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)
    rapdu: RAPDU = eos.get_async_response()
    public_key, address, chaincode = eos.parse_get_public_key_response(rapdu.data, True)

    check_get_public_key_resp(client, public_key, address, chaincode)

    # Check that with NO_CHAINCODE, value stay the same
    with eos.send_async_get_public_key_confirm(EOS_PATH, False):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)
    rapdu: RAPDU = eos.get_async_response()
    public_key_2, address_2, chaincode_2 = eos.parse_get_public_key_response(rapdu.data, False)
    assert public_key_2 == public_key
    assert address_2 == address
    assert chaincode_2 is None


def test_eos_get_public_key_confirm_refused(test_name, client, firmware, navigator):
    eos = EosClient(client)
    if firmware.device == "nanos":
        instructions = get_review_instructions(6)
    else:
        instructions = get_review_instructions(4)
    for chaincode_param in [True, False]:
        with eos.send_async_get_public_key_confirm(EOS_PATH, chaincode_param):
            client.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                           test_name,
                                           instructions)
        rapdu: RAPDU = eos.get_async_response()
        assert rapdu.status == ErrorType.USER_CANCEL
        assert len(rapdu.data) == 0


def load_transaction_from_file(transaction_filename):
    with open(TESTS_ROOT_DIR / "../corpus" / transaction_filename, "r") as f:
        obj = json.load(f)
    return Transaction().encode(obj)


def check_transaction(test_name, client, navigator, transaction_filename, num_screen_skip):
    signing_digest, message = load_transaction_from_file(transaction_filename)
    eos = EosClient(client)
    instructions = get_review_instructions(num_screen_skip)
    with eos.send_async_sign_message(EOS_PATH, message):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)
    rapdu: RAPDU = eos.get_async_response()
    eos.verify_signature(EOS_PATH, signing_digest, rapdu.data)


def test_eos_transaction_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction.json", 7)


def test_eos_transaction_buyrambytes_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_buyrambytes.json", 6)


def test_eos_transaction_buyram_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_buyram.json", 6)


def test_eos_transaction_deleteauth_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_deleteauth.json", 5)


def test_eos_transaction_linkauth_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_linkauth.json", 7)


# This transaction contains multiples actions which doesn't fit in one APDU.
# Therefore the app implementation test_name the user to validate the action
# fully contained in the first APDU before answering to it.
# Therefore we can't use the simple check_transaction() helper nor the
# send_async_sign_message() method and we need to do thing more manually.
def test_eos_transaction_newaccount_ok(test_name, client, navigator):
    signing_digest, message = load_transaction_from_file("transaction_newaccount.json")
    eos = EosClient(client)
    messages = split_message(EOS_PATH + message, MAX_CHUNK_SIZE)
    assert len(messages) == 2

    with eos._send_async_sign_message(messages[0], True):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name + "_part1",
                                       get_review_instructions(2) + get_review_instructions(7))
    rapdu: RAPDU = eos.get_async_response()

    with eos._send_async_sign_message(messages[1], False):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name + "_part2",
                                       get_review_instructions(6) + get_review_instructions(8))
    rapdu: RAPDU = eos.get_async_response()
    eos.verify_signature(EOS_PATH, signing_digest, rapdu.data)


def test_eos_transaction_refund_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_refund.json", 4)


def test_eos_transaction_sellram_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_sellram.json", 5)


# This transaction contains multiples actions which doesn't fit in one APDU.
# Therefore the app implementation test_name the user to validate the action
# fully contained in the first APDU before answering to it.
# Therefore we can't use the simple send_async_sign_message() method and we
# need to do thing more manually.
def test_eos_transaction_unknown_ok(test_name, client, navigator):
    signing_digest, message = load_transaction_from_file("transaction_unknown.json")
    eos = EosClient(client)
    messages = split_message(EOS_PATH + message, MAX_CHUNK_SIZE)

    with eos._send_async_sign_message(messages[0], True):
        client.raise_policy = RaisePolicy.RAISE_NOTHING
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       get_review_instructions(2))
    rapdu: RAPDU = eos.get_async_response()
    assert rapdu.status == 0x6A80


def test_eos_transaction_unlinkauth_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_unlinkauth.json", 6)


def test_eos_transaction_updateauth_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_updateauth.json", 19)


def test_eos_transaction_vote_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_vote.json", 33)


def test_eos_transaction_vote_proxy_ok(test_name, client, navigator):
    check_transaction(test_name, client, navigator, "transaction_vote_proxy.json", 5)


def test_eos_transaction_refused(test_name, client, navigator):
    _, message = load_transaction_from_file("transaction.json")
    eos = EosClient(client)
    instructions = get_review_instructions(8)
    with eos.send_async_sign_message(EOS_PATH, message):
        client.raise_policy = RaisePolicy.RAISE_NOTHING
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)
    rapdu: RAPDU = eos.get_async_response()
    assert rapdu.status == ErrorType.USER_CANCEL
    assert len(rapdu.data) == 0
