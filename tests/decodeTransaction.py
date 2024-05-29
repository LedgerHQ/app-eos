import binascii
import argparse
from asn1 import Decoder, Numbers
from termcolor import colored

#define CLA 0xD4
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00
#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5


def print_value(name, code):
    text = "{:>20}: {}".format(name, "{}")
    print(text.format(code))


def print_size(name, size):
    text = "{:>20}: {}".format(name, "{} {}")
    print(text.format(size, int(size, 16)))


def print_header(name):
    print("{:-^50}".format(name))


def print_path(fp, size, processing_size):
    total_path = ''
    for i in range(size):
        path = fp.read(8)
        print_value('path', path)
        total_path += path + ':'

    print_value('total path', total_path)
    processing_size -= 8*size
    return processing_size


def get_offset(stack):
    for i in range(0, len(stack)):
        return stack[i][0]

def print_offset(buffer, offset):
    bin_buf = binascii.unhexlify(buffer[offset:])
    decoder = Decoder()
    decoder.start(bin_buf)
    _, value = decoder.read()
    value = binascii.hexlify(value)

    idx = (buffer[offset:]).find(value.decode('utf-8'))+offset
    length = len(value)

    b0 = buffer[:offset]
    b1 = buffer[offset:idx]
    offset = idx
    b2 = buffer[offset:offset+length]
    offset += length
    b3 = buffer[offset:]

    print(b0, colored(b1, 'red'), colored(b2, 'blue'), b3, sep='')

codes = {
    'd4': 'cla',
    '02': 'instruction pub_key',
    '04': 'instruction sing',
    '06': 'instruction config',
}

p1_codes = {
    '01': 'p1_confirm',
    '00': 'p1_first',
    '80': 'p1_more'
}

p2_codes = {
    '01': 'p2_chaincode',
    '00': 'p2_no_chaincode',
}

class Context:
    def __init__(self):
        self.action_num = 0
        self.action_idx = 0
        self.auth_num = 0
        self.auth_idx = 0
        self.step = 0
        self.action_ready = False

    def __repr__(self):
        return 'step :{} act(i): {} act(n): {}, auth(i): {}, auth(n): {}'.format(self.step, self.action_idx, self.action_num, self.auth_idx, self.auth_num)


def next_step(context, value):
    context.step += 1
    return context


def store_action(context, value):
    value = int(value, 16)
    context.action_num = value
    return next_step(context, value)


def store_auth(context, value):
    value = int(value, 16)
    context.auth_num = value
    return next_step(context, value)


def goto(context, value):
    context.step = value
    return context


def define_next_auth_step(context, value):
    context.auth_idx += 1
    if context.auth_num <= context.auth_idx:
        return next_step(context, value)
    else:
        return goto(context, 12)

def define_next_action_step(context, value):
    context.action_idx += 1
    if context.action_num <= context.action_idx:
        return next_step(context, value)
    else:
        context.auth_idx = 0
        context.auth_num = 0
        context.action_ready = True
        return goto(context, 9)

parsing_header = {
    0:'chain id',
    1:'header expiration',
    2:'header block num',
    3:'header block prefix',
    4:'header cpu',
    5:'header net',
    6:'header delay',
    7:'cfa list size',
    8:'action list size',
    9:'action account',
    10:'action name',
    11:'auth list size',
    12:'auth actor',
    13:'auth permissiom',
    14:'action data size',
    15:'action data',
    16:'extention list size',
    17:'context free data'
}

parsing_actions = {
    0: next_step,
    1: next_step,
    2: next_step,
    3: next_step,
    4: next_step,
    5: next_step,
    6: next_step,
    7: next_step,
    8: store_action,
    9: next_step,
    10: next_step,
    11: store_auth,
    12: next_step, 
    13: define_next_auth_step,
    14: next_step,
    15: define_next_action_step,
    16: next_step,
    17: next_step,
}

parser = argparse.ArgumentParser()
parser.add_argument('--file', help="file to encoded transaction")
args = parser.parse_args()

if args.file is None:
    args.file = 'encoded_transaction.txt'

with open(args.file) as f:
    ctx = Context()
    while True:
        hex_data = ''

        print_header("INSTUCTION SET")
        # cla
        k = f.read(2)
        print_value(codes[k], k)

        # instuction
        k = f.read(2)
        print_value(codes[k], k)

        # p1
        p1 = f.read(2)
        print_value(p1_codes[p1], p1)

        # p2
        p2 = f.read(2)
        print_value(p2_codes[p2], p2)

        print_header("SIZE SET")

        total_size = f.read(2)
        print_size('total size', total_size)
        processing_size = total_size = int(total_size, 16)

        if p1 == '00':
            # path
            path_size = f.read(2)
            print_size('path size', path_size)
            path_size = int(path_size, 16)
        
            print_header('PATH SET')
            processing_size = print_path(f, path_size, processing_size)
        
        print_header("TX SET")
        print_value('processing size', processing_size)

        hex_data += f.readline()
        if hex_data[-1] == '\n':
            hex_data = hex_data[:-1]
        data = binascii.unhexlify(hex_data)
        decoder = Decoder()
        decoder.start(data)
        offset = get_offset(decoder.m_stack)

        while decoder.eof() is not True:

            try:
                buffer = decoder.read()
            except:
                hex_data = hex_data[offset*2:]
                break

            print_header(parsing_header[ctx.step])

            tag = buffer[0]
            value = binascii.hexlify(buffer[1])
            ctx = parsing_actions[ctx.step](ctx, value)

            print_offset(hex_data, offset*2)

            print_value('offset', offset)
            print_value('ber tag', tag)
            print_value('ber value', value)
            print_value('context', ctx)

            if ctx.action_ready:
                break

            offset = get_offset(decoder.m_stack)
