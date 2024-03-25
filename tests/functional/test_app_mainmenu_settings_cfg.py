from ragger.backend import SpeculosBackend
from ragger.navigator import NavInsID, NavIns

from apps.eos import EosClient
from utils import ROOT_SCREENSHOT_PATH

# Taken from the Makefile, to update every time the Makefile version is bumped
MAJOR = 1
MINOR = 4
PATCH = 5


def test_app_mainmenu_settings_cfg(firmware, backend, navigator, test_name):
    client = EosClient(backend)

    # Get appversion and "data_allowed parameter"
    # This works on both the emulator and a physical device
    data_allowed, version = client.send_get_app_configuration()
    assert data_allowed is False
    assert version == (MAJOR, MINOR, PATCH)

    # scoping navigation and next test to the emulator
    # navigation instructions are not applied to physical devices
    # without navigation instructions allow data remained unchanged
    #    no sense in running this test when there is no change
    if isinstance(backend, SpeculosBackend):
        # Navigate in the main menu and the setting menu
        # Change the "data_allowed parameter" value
        if firmware.device.startswith("nano"):
            instructions = [
                NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.LEFT_CLICK,
                NavInsID.BOTH_CLICK,
                NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.BOTH_CLICK
            ]
        else:
            instructions = [
                NavInsID.USE_CASE_HOME_INFO,
                NavIns(NavInsID.TOUCH, (200, 190)),  # Change setting value
                NavInsID.USE_CASE_SETTINGS_NEXT,
                NavInsID.USE_CASE_SUB_SETTINGS_PREVIOUS,
                NavInsID.USE_CASE_SUB_SETTINGS_EXIT
            ]
        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, instructions,
                                       screen_change_before_first_instruction=False)

        # Check that "data_allowed parameter" changed
        data_allowed, version = client.send_get_app_configuration()
        assert data_allowed is True
        assert version == (MAJOR, MINOR, PATCH)
