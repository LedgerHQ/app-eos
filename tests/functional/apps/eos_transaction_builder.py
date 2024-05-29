from asn1 import Encoder, Numbers
from base58 import b58decode
from binascii import unhexlify
from datetime import datetime
from hashlib import sha256
from struct import pack


def char_to_symbol(c):
    if c >= 'a' and c <= 'z':
        return ord(c) - ord('a') + 6
    if c >= '1' and c <= '5':
        return ord(c) - ord('1') + 1
    return 0


def encode_name(name):
    length = len(name)
    value = 0

    for i in range(0, 13):
        c = 0
        if i < length and i < 13:
            c = char_to_symbol(name[i])

        if i < 12:
            c &= 0x1f
            c <<= 64 - 5 * (i + 1)
        else:
            c &= 0x0f

        value |= c

    return pack('Q', value)


def symbol_from_string(p, name):
    length = len(name)
    result = 0
    for i in range(0, length):
        result |= ord(name[i]) << (8 * (i + 1))

    result |= p
    return result


def symbol_precision(sym):
    return pow(10, (sym & 0xff))


def encode_asset(asset):
    amount_str, symol_str = asset.split(' ')
    dot_pos = amount_str.find('.')

    # parse symbol
    if dot_pos != -1:
        precision_digit = len(amount_str) - dot_pos - 1
    else:
        precision_digit = 0

    sym = symbol_from_string(precision_digit, symol_str)

    # parse amount
    if dot_pos != -1:
        int_part = int(amount_str[:dot_pos])
        fract_part = int(amount_str[dot_pos+1:])
        if int_part < 0:
            fract_part *= -1
    else:
        int_part = int(amount_str)

    amount = int_part
    amount *= symbol_precision(sym)
    amount += fract_part

    data = pack('Q', amount)
    data += pack('Q', sym)
    return data


def encode_fc_uint(value):
    out = b''
    val = value
    while True:
        b = val & 0x7f
        val >>= 7
        b |= ((val > 0) << 7)
        out += chr(b).encode()

        if val == 0:
            break

    return out


def encode_public_key(data):
    data = str(data[3:])
    decoded = b58decode(data)
    decoded = decoded[:-4]
    parameters = pack('B', 0)
    parameters += decoded
    return parameters


def encode_auth(data):
    parameters = pack('I', data['threshold'])
    key_number = len(data['keys'])
    parameters += pack('B', key_number)
    for key in data['keys']:
        parameters += encode_public_key(key['key'])
        parameters += pack('H', key['weight'])
    parameters += pack('B', len(data['accounts']))
    for account in data['accounts']:
        parameters += encode_name(account['authorization']['actor'])
        parameters += encode_name(account['authorization']['permission'])
        parameters += pack('H', account['weight'])
    parameters += pack('B', len(data['waits']))
    for wait in data['waits']:
        parameters += pack('I', wait['wait'])
        parameters += pack('H', wait['weight'])
    return parameters


class Action:
    def encode(self, data, encoder):

        encoder.update(encode_name(data['account']))
        encoder.update(encode_name(data['name']))
        encoder.update(pack('B', len(data['authorization'])))

        for auth in data['authorization']:
            encoder.update(encode_name(auth['actor']))
            encoder.update(encode_name(auth['permission']))

        parameters = self.encode_action_parameters(data['data'])
        encoder.update(encode_fc_uint(len(parameters)))
        encoder.update(parameters)


class TransferAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['from'])
        parameters += encode_name(data['to'])
        parameters += encode_asset(data['quantity'])
        memo = data['memo']
        parameters += encode_fc_uint(len(memo))
        if len(memo) > 0:
            length = '{}s'.format(len(memo))
            parameters += pack(length, data['memo'].encode())

        return parameters


class VoteProducerAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['account'])
        parameters += encode_name(data['proxy'])
        parameters += encode_fc_uint(len(data['producers']))
        for producer in data['producers']:
            parameters += encode_name(producer)

        return parameters


class BuyRamAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['buyer'])
        parameters += encode_name(data['receiver'])
        parameters += encode_asset(data['tokens'])
        return parameters


class BuyRamBytesAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['buyer'])
        parameters += encode_name(data['receiver'])
        parameters += pack('I', data['bytes'])
        return parameters


class SellRamAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['receiver'])
        parameters += pack('Q', data['bytes'])
        return parameters


class UpdateAuthAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['account'])
        parameters += encode_name(data['permission'])
        parameters += encode_name(data['parent'])
        parameters += encode_auth(data['auth'])
        return parameters


class DeleteAuthAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['account'])
        parameters += encode_name(data['permission'])
        return parameters


class RefundAction(Action):
    def encode_action_parameters(self, data):
        return encode_name(data['account'])


class LinkAuthAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['account'])
        parameters += encode_name(data['contract'])
        parameters += encode_name(data['action'])
        parameters += encode_name(data['permission'])
        return parameters


class UnlinkAuthAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['account'])
        parameters += encode_name(data['contract'])
        parameters += encode_name(data['action'])
        return parameters


class NewAccountAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['creator'])
        parameters += encode_name(data['newact'])
        parameters += encode_auth(data['owner'])
        parameters += encode_auth(data['active'])
        return parameters


class DelegateAction(Action):
    def encode_action_parameters(self, data):
        parameters = encode_name(data['from'])
        parameters += encode_name(data['to'])
        parameters += encode_asset(data['stake_net_quantity'])
        parameters += encode_asset(data['stake_cpu_quantity'])
        parameters += bytes([0x01]) if data['transfer'] else bytes([0x00])
        return parameters


class UnknownAction(Action):
    def encode_action_parameters(self, data):
        # On purpose dummy and very long action to test the parser behavior
        data = data * 1000
        length = '{}s'.format(len(data))
        parameters = pack(length, data.encode())
        return parameters


def instantiate_action(name):
    if name == 'transfer':
        return TransferAction()
    elif name == 'voteproducer':
        return VoteProducerAction()
    elif name == 'buyram':
        return BuyRamAction()
    elif name == 'buyrambytes':
        return BuyRamBytesAction()
    elif name == 'sellram':
        return SellRamAction()
    elif name == 'updateauth':
        return UpdateAuthAction()
    elif name == 'deleteauth':
        return DeleteAuthAction()
    elif name == 'refund':
        return RefundAction()
    elif name == 'linkauth':
        return LinkAuthAction()
    elif name == 'unlinkauth':
        return UnlinkAuthAction()
    elif name == 'newaccount':
        return NewAccountAction()
    elif name == 'delegatebw':
        return DelegateAction()
    else:
        return UnknownAction()


class TransactionEncoder():
    def __init__(self):
        self.asn1_encoder = Encoder()
        self.sha = sha256()
        self.c = b""

    def start(self):
        self.asn1_encoder.start()

    def update(self, data):
        self.sha.update(data)
        self.asn1_encoder.write(data, Numbers.OctetString)
        self.c += data

    def digest(self):
        return self.sha.digest()

    def output(self):
        return self.asn1_encoder.output()


class Transaction():

    def encode(self, json):
        encoder = TransactionEncoder()
        encoder.start()

        encoder.update(unhexlify(json['chain_id']))

        body = json['transaction']

        expiration = int(datetime.strptime(body['expiration'],
                                           '%Y-%m-%dT%H:%M:%S').strftime("%s"))
        encoder.update(pack('I', expiration))
        encoder.update(pack('H', body['ref_block_num']))
        encoder.update(pack('I', body['ref_block_prefix']))
        encoder.update(pack('B', body['net_usage_words']))
        encoder.update(pack('B', body['max_cpu_usage_ms']))
        encoder.update(pack('B', body['delay_sec']))

        encoder.update(pack('B', len(body['context_free_actions'])))
        encoder.update(pack('B', len(body['actions'])))

        for action in body['actions']:
            act = instantiate_action(action["name"])
            act.encode(action, encoder)

        encoder.update(pack('B', len(body['transaction_extensions'])))
        encoder.update(unhexlify('00' * 32))

        return encoder.digest(), encoder.output()
