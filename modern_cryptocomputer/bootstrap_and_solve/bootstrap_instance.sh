#!/bin/sh

# CTF KEYS, KEEP PRIVATE, otherwise the challenge becomes trivial
GENESIS_PRIVATE_KEY=5KWTaHTyno6kpSNhFN6LFF2RoVRSZXCjMCMgVTXyyKhrWbD7qE1
GENESIS_PUBLIC_KEY=EOS7q6cC2mnLFhKjXhK4aRn7tQoHrd8XNkjPkUvCnyeks79bhSNG7

# Development keys
#GENESIS_PRIVATE_KEY=5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
#GENESIS_PUBLIC_KEY=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV


# IP address of the cleos node to configure (can be a remote one)
EOS_URL="${EOS_URL:-http://127.0.0.1:8888}"

set -e -v

# Reset : rm /root/eosio-wallet/default.wallet /root/cleos-default-wallet.password
if ! [ -e /root/cleos-default-wallet.password ] ; then
    cleos wallet create --file /root/cleos-default-wallet.password --name default
    cleos wallet open
    cleos wallet unlock --password "$(cat /root/cleos-default-wallet.password)"

    # Developer key
    echo 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3 | cleos wallet import

    echo "$GENESIS_PRIVATE_KEY" | cleos wallet import
else
    cleos wallet unlock --password "$(cat /root/cleos-default-wallet.password)" || :
fi

# https://developers.eos.io/welcome/v2.0/tutorials/bios-boot-sequence
# 1.7. Create important system accounts
cleos --url "$EOS_URL" create account eosio eosio.ram "$GENESIS_PUBLIC_KEY"
cleos --url "$EOS_URL" create account eosio eosio.ramfee "$GENESIS_PUBLIC_KEY"
cleos --url "$EOS_URL" create account eosio eosio.rex "$GENESIS_PUBLIC_KEY"
cleos --url "$EOS_URL" create account eosio eosio.stake "$GENESIS_PUBLIC_KEY"
cleos --url "$EOS_URL" create account eosio eosio.token "$GENESIS_PUBLIC_KEY"

#cleos --url "$EOS_URL" create account eosio eosio.bpay "$GENESIS_PUBLIC_KEY"
#cleos --url "$EOS_URL" create account eosio eosio.msig "$GENESIS_PUBLIC_KEY"
#cleos --url "$EOS_URL" create account eosio eosio.names "$GENESIS_PUBLIC_KEY"
#cleos --url "$EOS_URL" create account eosio eosio.saving "$GENESIS_PUBLIC_KEY"
#cleos --url "$EOS_URL" create account eosio eosio.vpay "$GENESIS_PUBLIC_KEY"

# 1.9. Install the eosio.token contract
cleos --url "$EOS_URL" set contract eosio.token /opt/eosio/eosio.contracts/build/contracts/eosio.token/

# 1.11. Create and allocate the SYS currency
cleos --url "$EOS_URL" push action eosio.token create '[ "eosio", "10000000000.0000 SYS" ]' -p eosio.token@active
cleos --url "$EOS_URL" push action eosio.token issue '[ "eosio", "1000000000.0000 SYS", "memo" ]' -p eosio@active

# 1.12. Set the eosio.system contract
curl --request POST \
    --url "$EOS_URL/v1/producer/schedule_protocol_feature_activations" \
    -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}'

sleep 5

RETRY_COUNT=0
while ! cleos --url "$EOS_URL" set contract eosio /opt/eosio/eosio.contracts/tests/test_contracts/old_versions/v1.8.3/eosio.system ; do
    # Error 3080006: Transaction took too long: try again
    echo >&2 "[$RETRY_COUNT] Set old system contract failed, retrying..."
    RETRY_COUNT=$((RETRY_COUNT+1))
    sleep 5
done

# GET_SENDER
cleos --url "$EOS_URL" push action eosio activate '["f0af56d2c5a48d60a4a5b5c903edfb7db3a736a94ed589d0b797df33ff9d3e1d"]' -p eosio

# FORWARD_SETCODE
cleos --url "$EOS_URL" push action eosio activate '["2652f5f96006294109b3dd0bbde63693f55324af452b799ee137a81a905eed25"]' -p eosio

# ONLY_BILL_FIRST_AUTHORIZER
cleos --url "$EOS_URL" push action eosio activate '["8ba52fe7a3956c5cd3a656a3174b931d3bb2abb45578befc59f283ecd816a405"]' -p eosio

# RESTRICT_ACTION_TO_SELF
cleos --url "$EOS_URL" push action eosio activate '["ad9e3d8f650687709fd68f4b90b41f7d825a365b02c23a636cef88ac2ac00c43"]' -p eosio

# DISALLOW_EMPTY_PRODUCER_SCHEDULE
cleos --url "$EOS_URL" push action eosio activate '["68dcaa34c0517d19666e6b33add67351d8c5f69e999ca1e37931bc410a297428"]' -p eosio

 # FIX_LINKAUTH_RESTRICTION
cleos --url "$EOS_URL" push action eosio activate '["e0fb64b1085cc5538970158d05a009c24e276fb94e1a0bf6a528b48fbc4ff526"]' -p eosio

 # REPLACE_DEFERRED
cleos --url "$EOS_URL" push action eosio activate '["ef43112c6543b88db2283a2e077278c315ae2c84719a8b25f25cc88565fbea99"]' -p eosio

# NO_DUPLICATE_DEFERRED_ID
cleos --url "$EOS_URL" push action eosio activate '["4a90c00d55454dc5b059055ca213579c6ea856967712a56017487886a4d4cc0f"]' -p eosio

# ONLY_LINK_TO_EXISTING_PERMISSION
cleos --url "$EOS_URL" push action eosio activate '["1a99a59d87e06e09ec5b028a9cbb7749b4a5ad8819004365d02dc4379a8b7241"]' -p eosio

# RAM_RESTRICTIONS
cleos --url "$EOS_URL" push action eosio activate '["4e7bf348da00a945489b2a681749eb56f5de00b900014e137ddae39f48f69d67"]' -p eosio

# WEBAUTHN_KEY
cleos --url "$EOS_URL" push action eosio activate '["4fca8bd82bbd181e714e283f83e1b45d95ca5af40fb89ad3977b653c448f78c2"]' -p eosio

# WTMSIG_BLOCK_SIGNATURES
cleos --url "$EOS_URL" push action eosio activate '["299dcb6af692324b899b39f16d5a530a33062804e41f09dc97e9f156b4476707"]' -p eosio

# Mandatory sleep, otherwise features are not enabled
sleep 5

RETRY_COUNT=0
while ! cleos --url "$EOS_URL" set contract eosio /opt/eosio/eosio.contracts/build/contracts/eosio.system ; do
    # Error 3080006: Transaction took too long: try again
    echo >&2 "[$RETRY_COUNT] Set new system contract failed, retrying..."
    RETRY_COUNT=$((RETRY_COUNT+1))
    sleep 5
done

# 2.2. Initialize system account
cleos --url "$EOS_URL" push action eosio init '["0", "4,SYS"]' -p eosio@active

# 2.4. Create staked accounts
for CONTRACT_NAME in $(cat "$(dirname -- "$0")/bip32_english.txt") ; do
    cleos --url "$EOS_URL" system newaccount eosio --transfer "$CONTRACT_NAME" EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV --stake-net "10000.0000 SYS" --stake-cpu "10000.0000 SYS" --buy-ram-kbytes 16
done
# Use a different key for the checker
cleos --url "$EOS_URL" system newaccount eosio --transfer flagchecker "$GENESIS_PUBLIC_KEY" --stake-net "100000000.0000 SYS" --stake-cpu "100000000.0000 SYS" --buy-ram-kbytes 8192

# Set hello as privileged
cleos --url "$EOS_URL" push action eosio setpriv '[flagchecker, 1]' -p eosio@active

# 3. Resign eosio account and system accounts
for SYSTEM_ACCOUNT in eosio eosio.ram eosio.ramfee eosio.rex eosio.stake eosio.token ; do
    cleos --url "$EOS_URL" push action eosio updateauth '{"account": "'"$SYSTEM_ACCOUNT"'", "permission": "owner", "parent": "", "auth": {"threshold": 1, "keys": [], "waits": [], "accounts": [{"weight": 1, "permission": {"actor": "eosio.prods", "permission": "active"}}]}}' -p "$SYSTEM_ACCOUNT"@owner
    cleos --url "$EOS_URL" push action eosio updateauth '{"account": "'"$SYSTEM_ACCOUNT"'", "permission": "active", "parent": "owner", "auth": {"threshold": 1, "keys": [], "waits": [], "accounts": [{"weight": 1, "permission": {"actor": "eosio.prods", "permission": "active"}}]}}' -p "$SYSTEM_ACCOUNT"@active
done

# Setup flag checker
(
    cd "$(dirname -- "$0")/flagchecker" && \
    eosio-cpp -abigen -o flagchecker.wasm flagchecker.cpp && \
    cleos --url "$EOS_URL" set contract flagchecker "$(pwd)" -p flagchecker@active
)
# Lock flagchecker
SYSTEM_ACCOUNT=flagchecker
cleos --url "$EOS_URL" push action eosio updateauth '{"account": "'"$SYSTEM_ACCOUNT"'", "permission": "owner", "parent": "", "auth": {"threshold": 1, "keys": [], "waits": [], "accounts": [{"weight": 1, "permission": {"actor": "eosio.prods", "permission": "active"}}]}}' -p "$SYSTEM_ACCOUNT"@owner
cleos --url "$EOS_URL" push action eosio updateauth '{"account": "'"$SYSTEM_ACCOUNT"'", "permission": "active", "parent": "owner", "auth": {"threshold": 1, "keys": [], "waits": [], "accounts": [{"weight": 1, "permission": {"actor": "eosio.prods", "permission": "active"}}]}}' -p "$SYSTEM_ACCOUNT"@active
