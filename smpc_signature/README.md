# Secure Multiparty Computation ECDSA Signature

Some organisation has been using ECDSA to sign some important data.
In order to avoid the secret key from being compromised, it was split in 16 fragments that were distributed to parties in a way that at least 5 parties need to share their fragments in order to recover the secret key.
Then, the organisation implemented a custom program that is able to sign data if enough parties agree, without requiring anyone to share their secret fragment.
This works thanks to a Secure Multiparty Computation algorithm based on the one used by the Ren Project (https://renproject.io/), which has been specified in https://github.com/renproject/rzl-mpc-specification/tree/33a60237eb5eb3ec617d125f533368683cfca628.

When signing some data, the encrypted packets that are exchanged between the participants are recorded into a log file.
A few days ago, some unknown attacker managed to breach into the network and to exfiltrate one of these log files (smpc_trace.log).
This attacker then managed to recover the secret key, even though he did not have any fragment!
So there is a critical vulnerability in the implementation.
Can you find it and recover the secret key?

When you find the secret key, please use get_flag.py to generate the flag.

## Reproduce the signature locally

```sh
# Install dependencies
pip install fastecdsa==2.1.5 noiseprotocol==0.3.1

# Generate a key pair, split amongst 16 parties (secret_share_01.json to secret_share_16.json)
./generate_split_key.py

# Sign some data, spawning a node for each secret share and running the SMPC signature algorithm
./sign_data.py
```
