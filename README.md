# Building and Testing EOS application 

Eos wallet application framework for Ledger devices

This follows the specification available in the doc/ folder

To use the generic wallet refer to `signTransaction.py`, `getPublicKey.py`
or Ledger EOS Wallet application [Anchor](https://www.greymass.com/anchor)

## Quick start guide

1) First make a new directory 
```shell
mkdir ledger-test
cd ledger-test
```

The [ledger-app-dev-tools](https://github.com/LedgerHQ/ledger-app-builder/pkgs/container/ledger-app-builder%2Fledger-app-dev-tools)
docker image contains all the required tools and libraries to **build**, **test** and **load** an application.

Ledger's general quick start guide to development and [integration with VS Code](https://developers.ledger.com/docs/device-app/beginner/vscode-extension)

2) You can download the development env from the ghcr.io docker repository

```shell
docker pull ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
```

3) You can then enter the docker, and the development environment. The eos-application code is mounted from the current working directory. Lets download that as well. Recommend changing to your development branch 

```shell
git clone https://github.com/eosnetworkfoundation/ledger-app.git
cd ledger-app
git checkout develop
```

4) Build per your platform

```shell
docker run --rm -ti -v "$(realpath .):/app" --user $(id -u $USER):$(id -g $USER) ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest
bash-5.1$ cd /app
bash-5.1$ BOLOS_SDK=$NANOSP_SDK make
```

5) Setup Ledger Build Env - required python virtual environment, needed for testing and side loading app onto device

```shell
cd /app
# create python virtual env 
python3 -m venv private_app_env --system-site-packages
# activate python virtual env
source private_app_env/bin/activate  
```

## Testing Via The Emulator 

###  `Linux`

```
cd ledger-app # enter directory for EOS Ledger App
sudo docker run --rm -ti -v "$(realpath .):/app" --user $(id -u):$(id -g) -v "/tmp/.X11-unix:/tmp/.X11-unix" -e DISPLAY=$DISPLAY ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
cd /app
source private_app_env/bin/activate
speculos build/nanosp/bin/app.elf --model nanosp
```

###  `MacOS`

XQuartz is the official X server implementation for macOS.

Download and install XQuartz from:
👉 https://www.xquartz.org
After installation, *restart* your computer for changes to take effect. 

#### Setup MacOS
1) update XQuartz->Settings to allow `network connection`. Shutdown XQuartz
2) run  `xhost + 127.0.0.1` to allow access XTerm Access 
3) Start with `open -a XQuartz` 

```
cd ledger-app # enter directory for EOS Ledger App
sudo docker run --rm -ti -v "$(pwd -P):/app" --user $(id -u):$(id -g) -v "/tmp/.X11-unix:/tmp/.X11-unix" -e DISPLAY="host.docker.internal:0" ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
cd /app
python3 -m venv private_app_env --system-site-packages
source private_app_env/bin/activate
speculos build/nanos2/bin/app.elf --model nanosp
```

## Compilation Options

From inside the container, use the following command to build the app:

```shell
bash-5.1$ cd /app
bash-5.1$ make DEBUG=1  # compile optionally with PRINTF
```

You can choose which device to compile and load for by setting the `BOLOS_SDK` environment variable to the following values:

- `BOLOS_SDK=$NANOX_SDK`
- `BOLOS_SDK=$NANOSP_SDK`
- `BOLOS_SDK=$STAX_SDK`
- `BOLOS_SDK=$FLEX_SDK`

The application's code will be available from inside the docker container,
you can proceed to the following compilation steps to build your app. To build with [clang static analyzer](https://github.com/LedgerHQ/ledger-app-builder/pkgs/container/ledger-app-builder%2Fledger-app-dev-tools#code-static-analysis) see this example.

```shell
sudo docker run --user "$(id -u)":"$(id -g)" --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
bash-5.1$ make scan-build
```

## Physical Device 

### Loading on a physical device

This step will vary slightly depending on your platform.
> Your physical device must be connected, unlocked and the screen showing the dashboard (not inside an application).

1) Download Ledger Live 
2) [Unlock developer mode and enable developer tools](https://developers.ledger.com/docs/ledger-live/discover/references/developer-mode)
3) Create a [manifest file](https://developers.ledger.com/docs/ledger-live/discover/references/manifest) for your app 

####  `Linux`
First make sure you have the proper udev rules added on your host.
See [udev-rules](https://github.com/LedgerHQ/udev-rules)

```
sudo wget -O /etc/udev/rules.d/20-hw1.rules https://raw.githubusercontent.com/LedgerHQ/udev-rules/master/20-hw1.rules
sudo udevadm control --reload-rules
```

## Running Tests

The application comes with functional tests,
implemented with Ledger's [Ragger](https://github.com/LedgerHQ/ragger) test framework.
They are located in the directory `tests/functional`.

Install the tests requirements:

```shell
export PYTHONPATH=/app/private_app_env/lib/python3.11/site-packages
pip install -r tests/functional/requirements.txt
pytest tests/functional/ --tb=short -v --device nanosp
```

You can run emulated tests for a specific device or for all devices. Set `--device` to `all` for all devices.
Use `--display` to see the emulated UI as the tests are run. The default mode runs the emulator in headless mode.

```shell
pytest tests/functional/ -v --tb=short --device=nanosp --display
```

This is a run in headless mode for `nanosp`
```shell
pytest tests/functional/ --tb=short -v --device nanosp
```

##  Issues and Solutions

#### Error Installing PyQt5

Check the site libraries, the library you export should match 
`python3 -m site`

Here are the steps to get around the following error :
```
Collecting PyQt5
  Downloading PyQt5-5.15.11.tar.gz (3.2 MB)
     ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 3.2/3.2 MB 17.1 MB/s eta 0:00:00
  Installing build dependencies ... done
  Getting requirements to build wheel ... done
  Preparing metadata (pyproject.toml) ... error
  error: subprocess-exited-with-error
  
  × Preparing metadata (pyproject.toml) did not run successfully.
 ```

You should not need to install PyQt5 as the dependency should already be install with the docker image. Make sure to use the `--system-site-packages` option when creating the python virtual environment. This will use the existing site packages that come with the docker container. Then make sure that the versions of `ragger` and `speculos` match the [Dockerfile](https://github.com/LedgerHQ/ledger-app-builder/blob/master/dev-tools/Dockerfile)

#### `Can't Find Module`

Typically the error will be something like 
`E   ModuleNotFoundError: No module named 'pycoin'`

## References

Full instructions are on [Ledger Developer Portal](https://developers.ledger.com/docs/device-app/develop/quickstart)

## Developer Notes

[Links to Documentation and GitHubs related to Ledger Development](./docs/Ledger-Developer-Notes.md)


