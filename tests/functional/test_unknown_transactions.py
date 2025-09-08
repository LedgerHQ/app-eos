from json import load
import pytest

from ledgered.devices import Device, DeviceType # type: ignore
from ragger.backend.interface import RaisePolicy
from ragger.bip import pack_derivation_path
from ragger.navigator import NavInsID
from ragger.utils import split_message
from ragger.backend import BackendInterface
from ragger.navigator.navigation_scenario import NavigateWithScenario
from test_app_mainmenu_settings_cfg import run_app_mainmenu_settings_cfg
from test_sign_cmd import get_nano_review_instructions

from apps.eos import EosClient, STATUS_OK, MAX_CHUNK_SIZE
from apps.eos_transaction_builder import Transaction
from utils import ROOT_SCREENSHOT_PATH, CORPUS_DIR
# Proposed EOS derivation paths for tests ###
VAULTA_PATH = "m/44'/194'/12345'"

def load_transaction_from_file(transaction_filename, subdir=None):
    if subdir:
        transaction_file_path = CORPUS_DIR / subdir / transaction_filename
    else:
        transaction_file_path = CORPUS_DIR / transaction_filename

    with transaction_file_path.open("r", encoding="utf-8") as f:
        obj = load(f)

    return Transaction().encode(obj)

def assemble_snapshot_folder_name(test_name, subdir, transaction_filename):
    if subdir:
        folder_name = test_name +"/"+ subdir +"/"+ transaction_filename.replace(".json", "")
    else:
        folder_name = test_name +"/"+ transaction_filename.replace(".json", "")

    return folder_name

# These files are in the corpus
unknown_trans = [('wampus','transaction_unknown.json'),
                ('wampus','transaction_badparam.json'),
                ('wampus','transaction_noparams.json'),
                ('wampus','transaction_nomemo.json')]

# This transaction contains multiples actions which fit in one APDU.
@pytest.mark.parametrize("transaction_filename", ['transaction_unknown.json'])
def test_sign_transaction_multiple_actions(test_name,
                                    device,
                                    backend,
                                    navigator,
                                    transaction_filename):

    folder_name = test_name + "/" + transaction_filename.replace(".json", "")

    _, message = load_transaction_from_file(transaction_filename)
    client = EosClient(backend)
    payload = pack_derivation_path(VAULTA_PATH) + message
    messages = split_message(payload, MAX_CHUNK_SIZE)

    if device.is_nano:
        instructions = get_nano_review_instructions(2)
    else:
        instructions = [NavInsID.USE_CASE_REVIEW_TAP]
        instructions.append(NavInsID.USE_CASE_REVIEW_CONFIRM)
    with client.send_async_sign_message_full(messages[0], True):
        backend.raise_policy = RaisePolicy.RAISE_NOTHING
        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       folder_name,
                                       instructions)
    rapdu = client.get_async_response()
    # assert stream fault , unknown actions not allowed
    assert rapdu.status == 0x6987

# This transaction contains multiples actions which fit in one APDU.
# first transaction is known and good
# second transaction is unknown
def process_transaction_with_mixed_actions(test_name: str,
                                    device: Device,
                                    backend: BackendInterface,
                                    scenario_navigator: NavigateWithScenario,
                                    subdir,
                                    transaction_filename: str,
                                    act1_arg_count=1,
                                    act2_arg_count=1,
                                    verbose=False):

    snapshot_folder_name = assemble_snapshot_folder_name(test_name, subdir, transaction_filename)

    # skip buggy test, only on STAX, correctly navigates, signs, and timesout.
    if device.type == DeviceType.STAX and transaction_filename == "mixed_transaction_noop_with_data_trans.json":
        return

    if verbose and device.is_nano:
        authorization_screens = 2
        added_page = 0
    elif verbose and not device.is_nano and transaction_filename == "mixed_transactions_known_unknown.json":
        authorization_screens = 1
        added_page = -1
    elif verbose and not device.is_nano:
        authorization_screens = 1
        added_page = 0
    elif not verbose and not device.is_nano and "with_data" in transaction_filename:
        added_page = 1
        authorization_screens = 0
    else:
        authorization_screens = 0
        added_page = 0

    _, message = load_transaction_from_file(transaction_filename, subdir)
    client = EosClient(backend)
    payload = pack_derivation_path(VAULTA_PATH) + message
    messages = split_message(payload, MAX_CHUNK_SIZE)

    if device.is_nano:
        # process initial identifying number of actions
        instructions = [NavInsID.RIGHT_CLICK] * 2
        instructions.append(NavInsID.BOTH_CLICK)
        # process first transaction
        instructions += [NavInsID.RIGHT_CLICK] * (3 + act1_arg_count + authorization_screens)
        instructions.append(NavInsID.BOTH_CLICK)
        # process second transaction
        instructions += [NavInsID.RIGHT_CLICK] * (3 + act2_arg_count + authorization_screens)
        instructions.append(NavInsID.BOTH_CLICK)
    elif device.type == DeviceType.FLEX:
        # flex screen wraps requiring additional screen and another tap
        taps = 3 + added_page + authorization_screens + (act1_arg_count) // 2 + authorization_screens + (act2_arg_count) // 2
        instructions = [NavInsID.USE_CASE_REVIEW_TAP] * taps
        instructions.append(NavInsID.USE_CASE_REVIEW_CONFIRM)
    else:
        taps = 3 + (act1_arg_count + authorization_screens)//2 + (act2_arg_count + authorization_screens) // 2
        instructions = [NavInsID.USE_CASE_REVIEW_TAP] * taps
        instructions.append(NavInsID.USE_CASE_REVIEW_CONFIRM)

    with client.send_async_sign_message_full(messages[0], True):
        scenario_navigator.navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       snapshot_folder_name,
                                       instructions)
    rapdu = client.get_async_response()
    assert rapdu.status == STATUS_OK

@pytest.mark.parametrize("transaction_filename", ['mixed_transactions_known_unknown.json'])
def test_sign_transaction_mixed_actions(test_name: str,
                                    device: Device,
                                    backend: BackendInterface,
                                    scenario_navigator: NavigateWithScenario,
                                    transaction_filename: str):

    # Allow Unknown Actions: navigate and turn on settings
    run_app_mainmenu_settings_cfg(device, backend, scenario_navigator.navigator)
    verbose = True
    if device.is_nano:
        action_one_args = 3 # buyram
        action_two_args = 3 # unknown
    else:
        action_one_args = 4 # buyram
        action_two_args = 3 # unknown

    process_transaction_with_mixed_actions(
        test_name,
        device,
        backend,
        scenario_navigator,
        None,
        transaction_filename,
        action_one_args,
        action_two_args,
        verbose)


# This transaction contains multiples actions which fit in one APDU.
# first transaction is known and good
# second transaction is unknown
# reject both; cancel transaction
@pytest.mark.parametrize("transaction_filename", ['mixed_transactions_known_unknown.json'])
def test_sign_mixed_actions_unknown_not_allowed(test_name: str,
                                    device: Device,
                                    backend: BackendInterface,
                                    scenario_navigator: NavigateWithScenario,
                                    transaction_filename: str):

    snapshot_folder_name = test_name + "/" + transaction_filename.replace(".json", "")

    _, message = load_transaction_from_file(transaction_filename)
    client = EosClient(backend)

    if device.is_nano:
        # process initial identifying number of actions
        instructions = [NavInsID.RIGHT_CLICK] * 2
        instructions.append(NavInsID.BOTH_CLICK)
        # process first transaction
        instructions += [NavInsID.RIGHT_CLICK] * 6
        instructions.append(NavInsID.BOTH_CLICK)
    elif device.type == DeviceType.FLEX:
        # flex screen wraps requiring additional screen and another tap
        instructions = [NavInsID.USE_CASE_REVIEW_TAP] * 4
    else:
        instructions = [NavInsID.USE_CASE_REVIEW_TAP] * 3

    with client.send_async_sign_message(VAULTA_PATH, message):
        backend.raise_policy = RaisePolicy.RAISE_NOTHING
        scenario_navigator.navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       snapshot_folder_name,
                                       instructions)
    rapdu = client.get_async_response()
    # assert stream fault , unknown actions not allowed
    assert rapdu.status == 0x6987

@pytest.mark.parametrize("subdir, transaction_filename", unknown_trans)
def test_malformed_transfer(test_name: str,
                            device: Device,
                            backend: BackendInterface,
                            scenario_navigator: NavigateWithScenario,
                            subdir: str,
                            transaction_filename: str):

    # Allow Unknown Actions: navigate and turn on settings
    run_app_mainmenu_settings_cfg(device, backend, scenario_navigator.navigator)

    snapshot_folder_name = assemble_snapshot_folder_name(test_name, subdir ,transaction_filename)

    signing_digest, message = load_transaction_from_file(transaction_filename, subdir)
    client = EosClient(backend)

    if device.is_nano:
        end_text = "^Sign$"
    else:
        end_text = "^Hold to sign$"
    with client.send_async_sign_message(VAULTA_PATH, message):
        scenario_navigator.review_approve(test_name=snapshot_folder_name, custom_screen_text=end_text)
    rapdu = client.get_async_response()
    client.verify_signature(VAULTA_PATH, signing_digest, rapdu.data)

@pytest.mark.parametrize("subdir, transaction_filename", unknown_trans)
def test_malformed_transfer_without_verbose(test_name: str,
                            device: Device,
                            backend: BackendInterface,
                            scenario_navigator: NavigateWithScenario,
                            subdir: str,
                            transaction_filename: str):

    # Allow Unknown Actions: navigate and turn on settings
    run_app_mainmenu_settings_cfg(device, backend, scenario_navigator.navigator, 'allow_unknown_actions')

    snapshot_folder_name = assemble_snapshot_folder_name(test_name, subdir ,transaction_filename)

    signing_digest, message = load_transaction_from_file(transaction_filename, subdir)
    client = EosClient(backend)

    if device.is_nano:
        end_text = "^Sign$"
    else:
        end_text = "^Hold to sign$"
    with client.send_async_sign_message(VAULTA_PATH, message):
        scenario_navigator.review_approve(test_name=snapshot_folder_name, custom_screen_text=end_text)
    rapdu = client.get_async_response()
    client.verify_signature(VAULTA_PATH, signing_digest, rapdu.data)

# Test Unknown action without allow Unknown actions set
# Note the navigator does not expect a screen change , and does not wait for instructions to produce one.
@pytest.mark.parametrize("subdir, transaction_filename", [('wampus','transaction_unknown.json')])
def test_unknown_action_not_allowed(test_name: str,
                            device: Device,
                            backend: BackendInterface,
                            scenario_navigator: NavigateWithScenario,
                            subdir: str,
                            transaction_filename: str):

    snapshot_folder_name = assemble_snapshot_folder_name(test_name, subdir ,transaction_filename)

    _, message = load_transaction_from_file(transaction_filename, subdir)
    client = EosClient(backend)

    if device.is_nano:
        # process initial identifying number of actions
        instructions = [NavInsID.RIGHT_CLICK]
    else:
        instructions = []
    with client.send_async_sign_message(VAULTA_PATH, message):
        backend.raise_policy = RaisePolicy.RAISE_NOTHING
        scenario_navigator.navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       snapshot_folder_name,
                                       instructions,
                                       screen_change_before_first_instruction=False)
    rapdu = client.get_async_response()
    # no change ; nothing presented
    assert rapdu.status == 0x6987

# Test Unknown action without allow Unknown actions set
# Note the navigator does not expect a screen change , and does not wait for instructions to produce one.
@pytest.mark.parametrize("subdir, transaction_filename", [('wampus','transaction_unknown.json')])
def test_unknown_action_allowed_not_verbose(test_name: str,
                            device: Device,
                            backend: BackendInterface,
                            scenario_navigator: NavigateWithScenario,
                            subdir: str,
                            transaction_filename: str):
    # Allow Unknown Actions, Verbose off: navigate and turn on settings
    run_app_mainmenu_settings_cfg(device, backend, scenario_navigator.navigator, 'allow_unknown_actions')
    snapshot_folder_name = assemble_snapshot_folder_name(test_name, subdir ,transaction_filename)

    signing_digest, message = load_transaction_from_file(transaction_filename, subdir)
    client = EosClient(backend)

    if device.is_nano:
        end_text = "^Sign$"
    else:
        end_text = "^Hold to sign$"
    with client.send_async_sign_message(VAULTA_PATH, message):
        scenario_navigator.review_approve(test_name=snapshot_folder_name, custom_screen_text=end_text)
    rapdu = client.get_async_response()
    assert rapdu.status == STATUS_OK
    client.verify_signature(VAULTA_PATH, signing_digest, rapdu.data)
