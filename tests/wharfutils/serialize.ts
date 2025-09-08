import { Serializer, ABI, Bytes, PackedTransaction, SignedTransaction } from '@wharfkit/antelope'
import * as fs from 'fs'
import * as path from 'path'

// Parse CLI arguments
const [, , actionDataPathArg, abiPathArg, formatArg, typeArg] = process.argv

if (!actionDataPathArg) {
  // note must convert to JS first
  console.error('Usage: node serialize.js <actionDataPath> [abiPath=./abi.json] [format=python-byte-string] [type=transaction]')
  process.exit(1)
}

const actionDataPath = actionDataPathArg
const abiPath = abiPathArg || './abi/transaction.abi.json'
const format = formatArg || 'python-byte-string'
const abiType = typeArg || 'transaction'

// Load files
const rawTransactionData = JSON.parse(fs.readFileSync(path.resolve(actionDataPath), 'utf-8'))
const abiJson = JSON.parse(fs.readFileSync(path.resolve(abiPath), 'utf-8'))

// Initialize ABI
const abi = ABI.from(abiJson)

// backward compatibility with older test json 
// older json includes chain-id and nested transaction object
// will return simple objects as well
function extractTransactionOrAll(obj: any): any {
  if (obj && typeof obj === 'object' && 'transaction' in obj) {
    return obj.transaction
  }
  return obj
}


function retrieveABI(account: string): ABI {

  switch (account) {
    case 'eosio.token':
      const tokenABIJson = JSON.parse(fs.readFileSync(path.resolve('./abi/eosio.token.abi.json'), 'utf-8'))
      return ABI.from(tokenABIJson)
      break
    case 'eosio':
      const eosioABIJson = JSON.parse(fs.readFileSync(path.resolve('./abi/eosio.abi.json'), 'utf-8'))
      return ABI.from(eosioABIJson)
      break
    case 'core.vaulta':
      var vaultABIJson = JSON.parse(fs.readFileSync(path.resolve('./abi/core.vaulta.abi.json'), 'utf-8'))
      return ABI.from(vaultABIJson)
      break
    case 'foocontract':
      var vaultABIJson = JSON.parse(fs.readFileSync(path.resolve('./abi/foocontract.abi.json'), 'utf-8'))
      return ABI.from(vaultABIJson)
      break
    default:
      var vaultABIJson = JSON.parse(fs.readFileSync(path.resolve('./abi/core.vaulta.abi.json'), 'utf-8'))
      return ABI.from(vaultABIJson)
  }
}

// serialize the action data with the given ABI 
// skip if hex data does not exist, hex_data means action data already serialized 
function serializeActionData(action: any): Bytes {
  if (action.account && action.name) {
    const abi: ABI = retrieveABI(action.account)
    return Serializer.encode({ object: action.data, abi: abi, type: action.name })
  }
  return new Bytes()
}



// easier compat with python 
function toPythonByteString(bytes: Bytes): string {
  return "b'" + Array.from(bytes.array)
    .map(b => `\\x${b.toString(16).padStart(2, '0')}`)
    .join('') + "'"
}

// ***************** MAIN **********************
// extract the transaction removing chain id 
const transactionData = extractTransactionOrAll(rawTransactionData)
// loop over the actions encoding them to bytes 
if (transactionData && Array.isArray(transactionData.actions)) {
  for (const action of transactionData.actions) {
    // sometimes hex_data is already provided
    // useful if we want to specify exact bytes to test edge cases
    // if not provided we need to create it
    if (!("hex_data" in action)) {
      // no data at all -> empty bytes
      // otherwise encode the action data
      if (!action.data) {
        action.hex_data = new Bytes()
      } else {
        action.hex_data = serializeActionData(action)
      }
    }
    // change data to bytes remove hex_data
    if (("hex_data" in action) && action.hex_data) {
      // process according to ABI
      action.data = action.hex_data
      delete action.hex_data
    }
  }
}

// Encode by ABI for given type, type defaults to 'transaction'
const encodedTransaction = Serializer.encode({
  object: transactionData,
  abi: abi,
  type: abiType
})

switch (format) {
  case 'python-byte-string':
    console.log(toPythonByteString(encodedTransaction))
    break
  case 'hex':
    console.log(String(encodedTransaction))
    break
  case 'utf8-string':
    console.log(encodedTransaction.utf8String)
    break
  case 'raw':
    console.log(encodedTransaction)
  default:
    console.error(`Unknown format: ${format}`)
    process.exit(1)
}
