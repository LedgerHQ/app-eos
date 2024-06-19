from ragger.backend import SpeculosBackend
from ragger.backend.interface import RaisePolicy
from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.backend import BackendInterface
from ragger.navigator.navigation_scenario import NavigateWithScenario

from apps.eos import EosClient, ErrorType

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


def test_get_public_key_confirm_accepted(backend: BackendInterface, scenario_navigator: NavigateWithScenario):
    client = EosClient(backend)
    with client.send_async_get_public_key_confirm(EOS_PATH, True):
        scenario_navigator.address_review_approve()

    response = client.get_async_response().data
    public_key, address, chaincode = client.parse_get_public_key_response(response, True)
    check_get_public_key_resp(backend, EOS_PATH, public_key, chaincode)

    # Check that with NO_CHAINCODE, value and screens stay the same
    with client.send_async_get_public_key_confirm(EOS_PATH, False):
        scenario_navigator.address_review_approve()

    response = client.get_async_response().data
    public_key_2, address_2, chaincode_2 = client.parse_get_public_key_response(response, False)
    assert public_key_2 == public_key
    assert address_2 == address
    assert chaincode_2 is None


# In this test we check that the GET_PUBLIC_KEY in confirmation mode replies an error if the user refuses
def test_get_public_key_confirm_refused(backend: BackendInterface, scenario_navigator: NavigateWithScenario):
    client = EosClient(backend)
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    for chaincode_param in [True, False]:
        with client.send_async_get_public_key_confirm(EOS_PATH, chaincode_param):
            scenario_navigator.address_review_reject()
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.USER_CANCEL
        assert len(rapdu.data) == 0
