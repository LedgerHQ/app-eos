# EOS Fuzzer

## Building and fuzzing

### Linux

```shell
BOLOS_SDK=/path/to/sdk/ cmake -Bbuild -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DFUZZ=1
cd build
make
```

```shell
mkdir ../corpus
cp ../ref_corpus/* ../corpus/
./fuzzer ../corpus/ -max_len=256
```

### Windows

```shell
$env:BOLOS_SDK = 'C:/path/to/sdk'       # (PowerShell)
cmake -Bbuild -GNinja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DFUZZ=1
cd build
ninja
```

```shell
mkdir ../corpus
copy ../ref_corpus/* ../corpus/*
./fuzzer.exe ../corpus/ -max_len=256
```

## Coverage information

Generating coverage:

```shell
python3 coverage.py
```

Will output an HTML report in `./coverage/index.html`.
