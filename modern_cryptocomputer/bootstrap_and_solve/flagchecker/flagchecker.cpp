/*
eosio-cpp -abigen -o flagchecker.wasm flagchecker.cpp
*/
#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

__attribute__((eosio_wasm_import))
extern "C" uint32_t crc32(const void *, uint32_t, uint32_t);

__attribute__((eosio_wasm_import))
extern "C" size_t get_secret_flag(void *, size_t);

class [[eosio::contract]] flagchecker : public contract {
  public:
    using eosio::contract::contract;

    [[eosio::action]]
    void checkhashflg(name user, std::string& hash_flag_hex) {
      require_auth(user);
      print("Hello, ", user, "!");

      uint32_t crc_name = crc32(user.to_string().c_str(), 0, user.length());
      if (crc_name != 0x4915557e) { // crc32(maze)
         print(" You are not allowed to call this contract.");
         return;
      }
      auto crc_name_hash = eosio::sha256((char *)&crc_name, 4);
      print(" easyflag=CTF{", crc_name_hash, "}");

      // Convert the hexadecimal into binary
      check(hash_flag_hex.length() == 64, "Unexpected SHA256 size");
      unsigned char hash_flag_bin[32];
      for (size_t i = 0; i < 64; i++) {
        char c = hash_flag_hex[i];
        unsigned int x;
        if (c >= '0' && c <= '9') {
          x = c - '0';
        } else if (c >= 'A' && c <= 'F') {
          x = c - ('A' - 10);
        } else if (c >= 'a' && c <= 'f') {
          x = c - ('a' - 10);
        } else {
          check(false, "Unexpected hex char");
          return;
        }
        if (i & 1) {
          hash_flag_bin[i >> 1] |= x;
        } else {
          hash_flag_bin[i >> 1] = x << 4;
        }
      }

      // Get the flag into some memory which is normally unavailable
      check(__builtin_wasm_grow_memory(1) == 1, "Unexpected memory size");

      char *flag = (char *)0x10000;
      size_t flag_size = get_secret_flag(flag, 0x10000);
      auto computed_hash = eosio::sha256(flag, flag_size).extract_as_byte_array();
      int is_different = 0;
      for (size_t i = 0; i < 32; i++) {
        is_different |= hash_flag_bin[i] ^ computed_hash[i];
      }
      if (is_different) {
         print(" Wrong flag, try again");
      } else {
         print(" Congrats! You found the flag!");
      }
    }
};
