# Running Tests

## Setup

General configuration for pytest is under `ledger-app/tests/functional/conftest.py`.
Configuration includes useful information like the `app name`, the `devices`, and the `ragger backends`.

### Ragger

First install `ragger`. It is a python package.

```shell
pip install ragger
```

See [Ragger Documentation](https://ledgerhq.github.io/ragger/) for additional information.

### Build The App

Follow the instructions in [Readme](../README.md#compile-your-ledger-app).

## Run The Emulator

To validate, run the app via speculos. Make sure that you run your emulator to match the build.

```shell
cd ledger-app
speculos build/nanosp/bin/app.elf
```

## Testing

### Install Packages

You will need to install several python packages

```shell
cd tests/functional
pip install -r requirements.txt
```

### Run Tests

You can run emulated tests for a specific device or for all devices. Set `--device` to `all` for all devices.
Use `--display` to see the emulated UI as the tests are run. The default mode runs the emulator in headless mode.

```shell
cd test/functional
pytest -v --tb=short --device=nanox --display
```

### CleanUp

remove the directory `ledger-app/tests/functional/snapshots-tmp/` to clean out the old snapshots
