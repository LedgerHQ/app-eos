from ragger.backend import SpeculosBackend
from ragger.backend.interface import RaisePolicy
from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.navigator import NavInsID, NavIns

from apps.eos import EosClient, ErrorType
from utils import ROOT_SCREENSHOT_PATH

# Proposed EOS derivation paths for tests ###
EOS_PATH = "m/44'/194'/12345'"


def check_get_public_key_resp(backend, path, public_key, chaincode):
    if isinstance(backend, SpeculosBackend):
        ref_public_key, ref_chain_code = calculate_public_key_and_chaincode(CurveChoice.Secp256k1, path)
        # Check against nominal Speculos seed expected results
        assert public_key.hex() == ref_public_key
        assert chaincode.hex() == ref_chain_code


def test_get_public_key_non_confirm(backend):
    client = EosClient(backend)

    rapdu = client.send_get_public_key_non_confirm(EOS_PATH, True)
    public_key, address, chaincode = client.parse_get_public_key_response(rapdu.data, True)
    check_get_public_key_resp(backend, EOS_PATH, public_key, chaincode)

    # Check that with NO_CHAINCODE, value stay the same
    rapdu = client.send_get_public_key_non_confirm(EOS_PATH, False)
    public_key_2, address_2, chaincode_2 = client.parse_get_public_key_response(rapdu.data, False)
    assert public_key_2 == public_key
    assert address_2 == address
    assert chaincode_2 is None


def test_get_public_key_confirm_accepted(firmware, backend, navigator, test_name):
    client = EosClient(backend)
    with client.send_async_get_public_key_confirm(EOS_PATH, True):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            instructions = [
                NavInsID.USE_CASE_REVIEW_TAP,
                NavIns(NavInsID.TOUCH, (200, 335)),
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR,
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS
            ]
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name,
                                           instructions)

    response = client.get_async_response().data
    public_key, address, chaincode = client.parse_get_public_key_response(response, True)
    check_get_public_key_resp(backend, EOS_PATH, public_key, chaincode)

    # Check that with NO_CHAINCODE, value and screens stay the same
    with client.send_async_get_public_key_confirm(EOS_PATH, False):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name,
                                           instructions)
    response = client.get_async_response().data
    public_key_2, address_2, chaincode_2 = client.parse_get_public_key_response(response, False)
    assert public_key_2 == public_key
    assert address_2 == address
    assert chaincode_2 is None


# In this test we check that the GET_PUBLIC_KEY in confirmation mode replies an error if the user refuses
def test_get_public_key_confirm_refused(firmware, backend, navigator, test_name):
    client = EosClient(backend)
    for chaincode_param in [True, False]:
        if firmware.device.startswith("nano"):
            with client.send_async_get_public_key_confirm(EOS_PATH, chaincode_param):
                backend.raise_policy = RaisePolicy.RAISE_NOTHING
                navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                          [NavInsID.BOTH_CLICK],
                                                          "Reject",
                                                          ROOT_SCREENSHOT_PATH,
                                                          test_name)
            rapdu = client.get_async_response()
            assert rapdu.status == ErrorType.USER_CANCEL
            assert len(rapdu.data) == 0
        else:
            instructions_set = [
                [
                    NavInsID.USE_CASE_REVIEW_REJECT,
                    NavInsID.USE_CASE_STATUS_DISMISS
                ],
                [
                    NavInsID.USE_CASE_REVIEW_TAP,
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL,
                    NavInsID.USE_CASE_STATUS_DISMISS
                ]
            ]
            for i, instructions in enumerate(instructions_set):
                for chaincode_param in [True, False]:
                    with client.send_async_get_public_key_confirm(EOS_PATH, chaincode_param):
                        backend.raise_policy = RaisePolicy.RAISE_NOTHING
                        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                                       test_name + f"/part{i}",
                                                       instructions)
                    rapdu = client.get_async_response()
                    assert rapdu.status == ErrorType.USER_CANCEL
                    assert len(rapdu.data) == 0
