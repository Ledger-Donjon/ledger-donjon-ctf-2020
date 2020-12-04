# Solving the challenge

Dumping all the blocks: [`dump_all_blocks.sh`](./dump_all_blocks.sh)

This leads to a smart contract:

```console
$ EOS_URL=http://127.0.0.1:8888
$ cleos --url "$EOS_URL" get code -c flagchecker.wast -a flagchecker.abi flagchecker
code hash: 621a0263e52bfb02e8c4fb13f1c8f139bcadcb4cfb629a33e5ff8c2dd3bb1275
saving wast to flagchecker.wast
saving abi to flagchecker.abi
```

Install the developer key in a wallet:

```sh
cleos wallet create --file /root/cleos-default-wallet.password --name default
cleos wallet open
cleos wallet unlock --password "$(cat /root/cleos-default-wallet.password)"
echo 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3 | cleos wallet import
```

Reversing the smart contract leads to the first flag, but brute-forcing usernames:

```console
$ cleos --url "$EOS_URL" push action flagchecker checkhashflg '[abandon, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"]' -p abandon@active
executed transaction: 81d39142419349c3cd43e7ef418f00f18974e3af0285cf7f842527b823d8268c  168 bytes  292 us
#   flagchecker <= flagchecker::checkhashflg    {"user":"abandon","hash_flag_hex":"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"...
>> Hello, abandon! You are not allowed to call this contract.

$ cleos --url "$EOS_URL" push action flagchecker checkhashflg '[maze, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"]' -p abandon@active
Error 3090004: Missing required authority
Ensure that you have the related authority inside your transaction!;
If you are currently using 'cleos push action' command, try to add the relevant authority using -p option.
Error Details:
missing authority of maze
pending console output:

$ cleos --url "$EOS_URL" push action flagchecker checkhashflg '[maze, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"]' -p maze@active
executed transaction: e21f01a7ebdf943a1b490ba0743486f33f833662037c5ceeef40f3d9bcd6fb10  168 bytes  861 us
#   flagchecker <= flagchecker::checkhashflg    {"user":"maze","hash_flag_hex":"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"}
>> Hello, maze! easyflag=CTF{04f5f3fbbc08dac23645890b03dd0d72fed6c5988621e62295610ff23a377e3b} Wrong flag, try again
```

Once the user is verified, exploiting the memory leak vuln using `crc32`:

```console
$ cd solution
$ eosio-cpp -abigen -o solution.wasm solution.cpp && cleos --url "$EOS_URL" set contract abandon "$(pwd)" -p abandon@active && cleos --url "$EOS_URL" push action abandon solve '[]' -p abandon@active
Warning, empty ricardian clause file
Warning, empty ricardian clause file
Warning, action <solve> does not have a ricardian contract
Reading WASM from /host/bootstrap_and_solve/solution/solution.wasm...
Publishing contract...
executed transaction: 59f77cad52dc722089794b214adcb6cfdf0943b7eb838f12bfcd0fad01252cee  848 bytes  506 us
#         eosio <= eosio::setcode               {"account":"abandon","vmtype":0,"vmversion":0,"code":"0061736d0100000001370b6000017f60027f7f0060037f...
#         eosio <= eosio::setabi                {"account":"abandon","abi":"0e656f73696f3a3a6162692f312e31000105736f6c76650000010000000000b523c50573...
warn  2020-10-06T15:44:54.880 cleos     main.cpp:513                  print_result        warning: transaction executed locally, but may not be confirmed by the network yet
executed transaction: 65d876c96f6dbf8d6a760b67463ffd3ab4d69dd881ab438948977954def08308  96 bytes  5680 us
#       abandon <= abandon::solve               ""
>> [Index 1: 3523407757] Reverse CRC32=0 [Index 2: 2920022741] Reverse CRC32=67...

$ python
>>> x='[Index 1: 3523407757] Reverse CRC32=0 [Index 2: 2920022741] Reverse CRC32=67...'
>>> import re;bytes(int(c) for c in re.findall(r'Reverse CRC32=([0-9]+)',x))
b'\x00CTF{S3cur1ty_with_C++_TeMpLaTeS_15_fragile}\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
*
$ printf 'CTF{S3cur1ty_with_C++_TeMpLaTeS_15_fragile}' |sha256sum
d52bdce40387e9ae5743d166c5b36133145e336b58aa87208de26310a8d41c3d  -

$ cleos --url "$EOS_URL" push action flagchecker checkhashflg '[maze, "d52bdce40387e9ae5743d166c5b36133145e336b58aa87208de26310a8d41c3d"]' -p maze@active
executed transaction: 414ca64ca2253c03418a274a7246ed91305e04ff93eb5df7f52e7720f9d7ebf9  168 bytes  334 us
#   flagchecker <= flagchecker::checkhashflg    {"user":"maze","hash_flag_hex":"d52bdce40387e9ae5743d166c5b36133145e336b58aa87208de26310a8d41c3d"}
>> Hello, maze! easyflag=CTF{04f5f3fbbc08dac23645890b03dd0d72fed6c5988621e62295610ff23a377e3b} Congrats! You found the flag!
```

## Flags

```text
Easy Modern Cryptocomputer: CTF{04f5f3fbbc08dac23645890b03dd0d72fed6c5988621e62295610ff23a377e3b}

Hard Modern Cryptocomputer: CTF{S3cur1ty_with_C++_TeMpLaTeS_15_fragile}
```
