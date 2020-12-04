/*
eosio-cpp -abigen -o solution.wasm solution.cpp && cleos --url "$EOS_URL" set contract abandon "$(pwd)" -p abandon@active && cleos --url "$EOS_URL" push action abandon solve '[]' -p abandon@active

eosio-wasm2wast solution.wasm

python
>>> import re;bytes(int(x) for x in re.findall(r'Reverse CRC32=([0-9]+)',result))
b'\x00No flag provided\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
*/
#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

__attribute__((eosio_wasm_import))
extern "C" uint32_t crc32(const void *, uint32_t, uint32_t);

__attribute__((eosio_wasm_import))
extern "C" size_t get_secret_flag(void *, size_t);

static uint32_t crc32_byte(uint32_t data, uint32_t value) {
    value = ~value;
    value ^= data;
    for (unsigned int bitpos = 0; bitpos < 8; bitpos ++) {
       value = (value >> 1) ^ (0xEDB88320 & -(value & 1));
    }
    return ~value;
}
static uint8_t reverse_crc32_byte(uint32_t from_value, uint32_t to_value) {
    for (uint32_t test_byte = 0; test_byte < 0x100; test_byte++) {
        if (crc32_byte(test_byte, from_value) == to_value) {
            print(" Reverse CRC32=", test_byte, " ");
        }
    }
    return (uint8_t)0;
}

class [[eosio::contract]] solution : public contract {
  public:
    using eosio::contract::contract;

    [[eosio::action]]
    void solve() {
      uint32_t last_value = 0;
      for (uint32_t index = 1; index < 60; index++) {
        uint32_t value = crc32((const void *)0xffff, 0, index);
        print("[Index ", index, ": ", value, "]");
        reverse_crc32_byte(last_value, value);
        last_value = value;
      }
    }
};
