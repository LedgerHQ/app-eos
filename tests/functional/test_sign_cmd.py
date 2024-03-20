import pytest
from json import load

from ragger.backend.interface import RaisePolicy
from ragger.bip import pack_derivation_path
from ragger.navigator import NavInsID
from ragger.utils import split_message

from apps.eos import EosClient, ErrorType, MAX_CHUNK_SIZE
from apps.eos_transaction_builder import Transaction
from utils import ROOT_SCREENSHOT_PATH, CORPUS_DIR, CORPUS_FILES

# Proposed EOS derivation paths for tests ###
EOS_PATH = "m/44'/194'/12345'"


def load_transaction_from_file(transaction_filename):
    with open(CORPUS_DIR / transaction_filename, "r") as f:
        obj = load(f)
    return Transaction().encode(obj)


def check_transaction(test_name, firmware, backend, navigator, transaction_filename):
    signing_digest, message = load_transaction_from_file(transaction_filename)
    client = EosClient(backend)
    with client.send_async_sign_message(EOS_PATH, message):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            navigator.navigate_until_text_and_compare(NavInsID.USE_CASE_REVIEW_TAP,
                                                      [NavInsID.USE_CASE_REVIEW_CONFIRM,
                                                       NavInsID.USE_CASE_STATUS_DISMISS],
                                                      "Hold to sign",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
    response = client.get_async_response().data
    client.verify_signature(EOS_PATH, signing_digest, response)


# Remove corner case transaction that are handled separately
transactions = list(CORPUS_FILES)
transactions.remove("transaction_newaccount.json")
transactions.remove("transaction_unknown.json")


@pytest.mark.parametrize("transaction_filename", transactions)
def test_sign_transaction_accepted(test_name, firmware, backend, navigator, transaction_filename):
    folder_name = test_name + "/" + transaction_filename.replace(".json", "")
    check_transaction(folder_name, firmware, backend, navigator, transaction_filename)


def test_sign_transaction_refused(test_name, firmware, backend, navigator):
    signing_digest, message = load_transaction_from_file("transaction.json")
    client = EosClient(backend)
    if firmware.device.startswith("nano"):
        with client.send_async_sign_message(EOS_PATH, message):
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Cancel",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.USER_CANCEL
        assert len(rapdu.data) == 0
    else:
        for i in range(4):
            instructions = [NavInsID.USE_CASE_REVIEW_TAP] * i
            instructions += [NavInsID.USE_CASE_REVIEW_REJECT,
                             NavInsID.USE_CASE_CHOICE_CONFIRM,
                             NavInsID.USE_CASE_STATUS_DISMISS]
            with client.send_async_sign_message(EOS_PATH, message):
                backend.raise_policy = RaisePolicy.RAISE_NOTHING
                navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                               test_name + f"/part{i}",
                                               instructions)
            rapdu = client.get_async_response()
            assert rapdu.status == ErrorType.USER_CANCEL
            assert len(rapdu.data) == 0


def get_nano_review_instructions(num_screen_skip):
    instructions = [NavInsID.RIGHT_CLICK] * num_screen_skip
    instructions.append(NavInsID.BOTH_CLICK)
    return instructions


# This transaction contains multiples actions which doesn't fit in one APDU.
# Therefore the app implementation ask the user to validate the action
# fully contained in the first APDU before answering to it.
# Therefore we can't use the simple check_transaction() helper nor the
# send_async_sign_message() method and we need to do thing more manually.
def test_sign_transaction_newaccount_accepted(test_name, firmware, backend, navigator):
    signing_digest, message = load_transaction_from_file("transaction_newaccount.json")
    client = EosClient(backend)
    payload = pack_derivation_path(EOS_PATH) + message
    messages = split_message(payload, MAX_CHUNK_SIZE)
    assert len(messages) == 2

    if firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(2) + get_nano_review_instructions(7)
    else:
        instructions = [NavInsID.USE_CASE_REVIEW_TAP] * 5
    with client._send_async_sign_message(messages[0], True):
        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       test_name + "/part1",
                                       instructions)

    if firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(6) + get_nano_review_instructions(8)
    else:
        instructions = [NavInsID.USE_CASE_REVIEW_TAP] * 6
        instructions.append(NavInsID.USE_CASE_REVIEW_CONFIRM)
        instructions.append(NavInsID.USE_CASE_STATUS_DISMISS)
    with client._send_async_sign_message(messages[1], False):
        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       test_name + "/part2",
                                       instructions)
    response = client.get_async_response().data
    client.verify_signature(EOS_PATH, signing_digest, response)


# This transaction contains multiples actions which doesn't fit in one APDU.
# Therefore the app implementation ask the user to validate the action
# fully contained in the first APDU before answering to it.
# Therefore we can't use the simple send_async_sign_message() method and we
# need to do thing more manually.
def test_sign_transaction_unknown_fail(test_name, firmware, backend, navigator):
    signing_digest, message = load_transaction_from_file("transaction_unknown.json")
    client = EosClient(backend)
    payload = pack_derivation_path(EOS_PATH) + message
    messages = split_message(payload, MAX_CHUNK_SIZE)

    if firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(2)
    else:
        instructions = [NavInsID.USE_CASE_REVIEW_TAP]
    with client._send_async_sign_message(messages[0], True):
        backend.raise_policy = RaisePolicy.RAISE_NOTHING
        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       test_name,
                                       instructions)
    rapdu = client.get_async_response()
    assert rapdu.status == 0x6A80
