from typing import List
from pathlib import Path
import re
from ragger.backend import SpeculosBackend
from ragger.navigator import NavInsID, NavIns

from apps.eos import EosClient
from utils import ROOT_SCREENSHOT_PATH

def _read_makefile() -> List[str]:
    """Read lines from the parent Makefile """

    parent = Path(__file__).parent.parent.parent.resolve()
    makefile = f"{parent}/Makefile"
    print(f"Makefile: {makefile}")
    with open(makefile, "r", encoding="utf-8") as f_p:
        lines = f_p.readlines()
    return lines

def _verify_version(version: str) -> None:
    """Verify the app version, based on defines in Makefile

    Args:
        Version (str): Version to be checked
    """

    vers_dict = {}
    vers_str = (0, 0, 0)
    lines = _read_makefile()
    version_re = re.compile(r"^APPVERSION_(?P<part>\w)\s?=\s?(?P<val>\d*)", re.I)
    for line in lines:
        info = version_re.match(line)
        if info:
            dinfo = info.groupdict()
            vers_dict[dinfo["part"]] = int(dinfo["val"])
    try:
        vers_str = (vers_dict['M'], vers_dict['N'], vers_dict['P'])
    except KeyError:
        pass
    assert version == vers_str


def test_app_mainmenu_settings_cfg(firmware, backend, navigator, test_name):
    client = EosClient(backend)

    # Get appversion and "data_allowed parameter"
    # This works on both the emulator and a physical device
    data_allowed, version = client.send_get_app_configuration()
    assert data_allowed is False
    _verify_version(version)

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
                NavInsID.USE_CASE_SETTINGS_PREVIOUS,
                NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT
            ]
        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, instructions,
                                       screen_change_before_first_instruction=False)

        # Check that "data_allowed parameter" changed
        data_allowed, version = client.send_get_app_configuration()
        assert data_allowed is True
        _verify_version(version)
