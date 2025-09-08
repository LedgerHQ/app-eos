from typing import List
from pathlib import Path
import re
from ragger.backend import SpeculosBackend
from ragger.navigator import NavInsID, NavIns
from ledgered.devices import DeviceType

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

def run_app_mainmenu_settings_cfg(device, backend, navigator, setting='all', test_name=None):
    client = EosClient(backend)

    # Get appversion and "data_allowed parameter"
    # This works on both the emulator and a physical device
    unknown_allowed, is_verbose, version = client.send_get_app_configuration()
    assert unknown_allowed is False
    _verify_version(version)

    # scoping navigation and next test to the emulator
    # navigation instructions are not applied to physical devices
    # without navigation instructions allow data remained unchanged
    #    no sense in running this test when there is no change
    if isinstance(backend, SpeculosBackend):
        # Navigate in the main menu and the setting menu
        # Change the "data_allowed parameter" value
        if device.is_nano:
            instructions = [
                NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.LEFT_CLICK,
                NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK
            ]

            if setting in ('all','verbose'):
                instructions.extend([
                    NavInsID.BOTH_CLICK,
                    NavInsID.RIGHT_CLICK,
                    NavInsID.LEFT_CLICK
                    ])
            else:
                instructions.append(NavInsID.LEFT_CLICK)

            if setting in ('all','allow_unknown_actions'):
                instructions.append(NavInsID.BOTH_CLICK)

            instructions.extend([
                NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK,
                NavInsID.BOTH_CLICK
            ])
        elif device.type == DeviceType.FLEX:
            instructions = [NavInsID.USE_CASE_HOME_INFO]

            if setting in ('all','allow_unknown_actions'):
                instructions.append(NavIns(NavInsID.TOUCH, (200, 190)))  # Change setting value

            instructions.append(NavInsID.USE_CASE_SETTINGS_NEXT)

            if setting in ('all','verbose'):
                instructions.append(NavIns(NavInsID.TOUCH, (200, 190)))  # Change setting value

            instructions.extend([
                NavInsID.USE_CASE_SETTINGS_PREVIOUS,
                NavInsID.USE_CASE_SETTINGS_NEXT,
                NavInsID.USE_CASE_SETTINGS_NEXT,
                NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT
            ])
        else:
            instructions = [NavInsID.USE_CASE_HOME_INFO]

            if setting in ('all','allow_unknown_actions'):
                instructions.append(NavIns(NavInsID.TOUCH, (200, 190)))  # Change setting value
            if setting in ('all','verbose'):
                instructions.append(NavIns(NavInsID.TOUCH, (200, 360)))  # Change setting value

            instructions.extend([
                NavInsID.USE_CASE_SETTINGS_NEXT,
                NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT
            ])
        # test_name null means this is a config change event, not a test
        if test_name:
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, instructions,
                                           screen_change_before_first_instruction=False)
        else:
            navigator.navigate(instructions,screen_change_before_first_instruction=False)

        # Check that "data_allowed parameter" changed
        unknown_allowed, is_verbose, version = client.send_get_app_configuration()
        if setting in ('all','allow_unknown_actions'):
            assert unknown_allowed is True
        if setting in ('all','verbose'):
            assert is_verbose is True
        _verify_version(version)

def test_app_mainmenu_settings_cfg(device, backend, navigator):
    run_app_mainmenu_settings_cfg(device, backend, navigator, 'all', "test_app_mainmenu_settings_cfg")
