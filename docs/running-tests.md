# Running Tests

## Setup

General configuration for pytest is under `ledger-app/tests/functional/conftest.py`. Configuration includes useful information like the `app name`, the `devices`, and the `ragger backends`.

### Ragger
First install `ragger`. It is a python package. May be [installed via pip](https://ledgerhq.github.io/ragger/installation.html)

Alternatively you can install directly from the github repo.

```
git clone https://github.com/LedgerHQ/ragger
cd ragger
pip install --extra-index-url https://test.pypi.org/simple/ '.[all_backends]'
```

See [Ragger Documentation](https://ledgerhq.github.io/ragger/) for addtional information.

### Build The App

Follow the instructions in [Readme](../README.md#compile-your-ledger-app).

After each build rename the binaries from `app.elf` to `eos_<device>.elf`. Device is one of the three listed here *nano*, *nanox*, *nanosp*. You will find the binaries under `/bin` directory.

### Setup Binaries

Copy the compiled binaries to `tests/elfs` directory, create the directory if necessary.
```
mkdir -p tests/elfs/
cd bin
sudo mv app.elf eos_<device>.elf
cp eos_*.elf ../tests/elfs
cd ../
```

## Repeat
Build and copy the binary for each device to `tests/elfs`

There should be three binaries under `tests/elfs`
- eos_nano.elf
- eos_nanox.elf
- eos_nanosp.elf

## Run The Emulator

To validate run the app via speculos. Make sure that you run your emulator to match the build. You pass the correct device in with the `-m` option.

Note your path to `speculos.py` may differ.

```
cd ledger-app
../../ledgerHQ/speculos/speculos.py -m nanosp tests/elfs/eos_naosp.elf
```

## Testing

### Install Packages

You will need to install several python packages
```
pip install pytest pycoin asn1 base58
```

### Run Tests

You can run emulated tests for a specific device or for all devices. Set `--device` to `all` for all devices.
Use `--display` to see the emulated UI as the tests are run. The default mode runs the emulator in headless mode.  
```
cd test/functional
pytest -v --tb=short --device=nanox --display
```

### CleanUp

remove the directory `ledger-app/tests/functional/snapshots-tmp/` to clean out the old snapshots
