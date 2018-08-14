#pragma once
#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/system.h>
#include <eosiolib/transaction.h>
#include <eosiolib/types.h>

#include <climits>
#include <string>
#include <utility>
#include <type_traits>

#include "constants.hpp"
#include "types.hpp"

#undef EOSIO_ABI
#define EOSIO_ABI(TYPE, MEMBERS) \
extern "C" {  \
    void apply( uint64_t receiver, uint64_t sender, uint64_t action ) { \
        auto self = receiver; \
        if( action == N(onerror)) { \
            /* onerror is only valid if it is for the "eosio" sender account and authorized by "eosio"'s "active permission */ \
            eosio_assert(sender == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
        } \
        TYPE thiscontract( self ); \
        if(sender == self || action == N(onerror)) { \
            switch( action ) { \
                EOSIO_API( TYPE, MEMBERS ) \
            } \
        } \
        else { \
            thiscontract.on_notification(sender, action); \
        } \
    } \
}


namespace eosram {
    using eosio::asset;
    using eosio::extended_asset;
    using eosio::symbol_type;

    static void asset_assert(const asset& asset, const symbol_type& sym,  const char* msg)
    {
        eosio_assert(asset.symbol.is_valid() , "Invalid symbol name" );
        eosio_assert(asset.symbol == sym     , msg                   );
        eosio_assert(asset.is_valid()        , "Invalid quantity."   );
    }

    static void asset_assert(const asset& asset, const symbol_type& sym1, const symbol_type& sym2,  const char* msg)
    {
        eosio_assert(asset.symbol.is_valid() , "Invalid symbol name" );
        eosio_assert(asset.symbol == sym1  || asset.symbol == sym2 , msg);
        eosio_assert(asset.is_valid()        , "Invalid quantity.");
    }

    /* Inline transfers token from snder to recipient */
    static void transfer_token(const account_name from, const account_name to, const extended_asset amount, std::string memo = "")
    {
        eosio_assert(amount.is_valid(), "Cannot transfer invalid amount!" );
        dispatch_inline(amount.contract,  N(transfer), {{ from, N(active) }}, 
            std::make_tuple(from, to, static_cast<const asset&>(amount), std::move(memo))
        );
    }

    /* Returns current transaction id */
    static transaction_id_type get_txid() 
    {
        auto size = transaction_size();
        char raw_tx[size];
        uint32_t read = read_transaction(raw_tx, size);
        eosio_assert(size == read, "read_transaction failed");

        transaction_id_type txid;
        sha256(raw_tx, read, &txid);
        return txid;
    }

    /* Returns order id from transaction id */
    static order_id_t get_order_id(const transaction_id_type& txid)
    {
        static_assert(sizeof(order_id_t) == sizeof(uint64_t), "size mismatch!");
        const uint64_t* p64 = reinterpret_cast<const uint64_t*>(&txid);
        order_id_t order_id_l = p64[0] ^ p64[1];
        order_id_t order_id_r = p64[2] ^ p64[3];
        return order_id_l ^ order_id_r;
    }

    static bool ttl_infinite(ttl_t ttl) {
        return ttl <= infinite_ttl;
    }

    static bool ttl_valid(ttl_t ttl) {
        return ttl_infinite(ttl) || ttl >= min_ttl;
    }

   /**
    * Returns true if order has expired, otherwise false.
    * Order expires it's expiration time if greater than 0 and current time
    * is higher or equal than order expiration time.
    * 
    * @param order expiration time
    * @returns true/false
    */
    static bool order_expired(ttl_t time) {
        return time != 0 && now() >= time;
    }

   /** 
    * Returns order's expiration time based on current time and ttl.
    * If ttl is infinite than experation time is 0.
    * 
    * @param ttl
    * @returns order expiration time based on current time
    */
    static uint32_t get_order_expiration_time(ttl_t ttl) 
    {   
        eosio_assert(ttl_valid(ttl), "Invlid order ttl!");
        return ttl_infinite(ttl) ? 0 : now() + ttl;
    }

    // Converts char to byte (code ref taken from eosio/fc)
    static uint8_t from_hex(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        eosio_assert(false, "Invalid hex character!");
        return 0;
    }

    // Converts byte data to hex string (code ref taken from eosio/fc)
    static std::string to_hex(const char* d, uint32_t s)
    {
        std::string r;
        const char *to_hex = "0123456789abcdef";
        uint8_t *c = (uint8_t *)d;
        for (uint32_t i = 0; i < s; ++i) {
            (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
        }
        return r;
    }

    // Converts T to hex string
    template<typename T, typename std::enable_if<std::is_trivial<T>::value>* = nullptr>
    static std::string to_hex(const T& t) {
        return to_hex(reinterpret_cast<const char*>(&t), sizeof(t));
    }

    // Converts hex string to bytes (code ref taken from eosio/fc)
    static size_t from_hex(const std::string& hex_str, char* out_data, size_t out_data_len, std::size_t hex_str_ofs = 0)
    {
        auto i = hex_str.begin() + hex_str_ofs;
        uint8_t *out_pos = (uint8_t *)out_data;
        uint8_t *out_end = out_pos + out_data_len;
        while (i != hex_str.end() && out_end != out_pos)
        {
            *out_pos = from_hex(*i) << 4;
            ++i;
            if (i != hex_str.end())
            {
                *out_pos |= from_hex(*i);
                ++i;
            }
            ++out_pos;
        }
        return out_pos - (uint8_t *)out_data;
    }

    /** Converts hex string to T */
    template<typename T, std::enable_if<std::is_trivial<T>::value>* = nullptr>
    static T from_hex(const std::string& hex_str, std::size_t hex_str_ofs = 0)
    {
        T t;
        auto read = from_hex(hex_str, reinterpret_cast<char *>(&t), sizeof(t), hex_str_ofs);
        eosio_assert(read == sizeof(t), "Invalid hex string");
        return t;
    }

    /** Checks if string contains sub string at position pos. */
    bool str_contains_at(std::string_view str, std::size_t pos, std::string_view str2)
    {
        return str.find(str2, pos) == pos;
    }

    /** Returns number of digits in number */
    uint32_t num_digits(uint32_t num) {
        return num /10 > 0 ? 1 +  num_digits(num / 10) : 1;
    }

    /** Converts number to string */
    static std::string num_to_str(int32_t num)
    {
        uint8_t neg = false;
        if(num < 0)
        {
            neg = num == INT_MIN ? 0x2 : 0x1;
            num = -num - (neg & 0x2 ? 1 : 0); // sub 1 if num is INT_MIN
        } 

        std::string str(num_digits(num) + (neg ? 1 : 0), '0');
        auto it = str.rbegin();
        do 
        {
            *it = '0' + num % 10;
            ++it;
        } while((num /= 10) > 0);

        if(neg) *it = '-';
        if(neg & 0x2) *str.rbegin() = *str.rbegin() + 1; // add 1 if num is INT_MIN (e.g. -...83647 -> -...83648)
        return str;
    }

   /**
    * Converts string to nubmer.
    * First charecter in string has to be digit or +/- sign
    * otherwise function returns 0 and sets end_pos arg to 0.
    * 
    * @param string to convert to number.
    * @param pointer to the size_t variable which will be set to the offset of character following the number.
    *        If function sets this parameter to 0 it means no number was found at the begining of the string.
    *
    * @returns number
    */
    static int32_t str_to_num(std::string_view str, std::size_t* end_pos = nullptr)
    {
        if(str.empty()) 
        {
            if(end_pos) *end_pos = 0;
            return 0L;
        }

        std::size_t pos = 0;
        uint64_t n = 0ULL, neg = 0ULL;
        switch (str.at(pos)) 
        {
            case '-': neg = -1;
            case '+': pos++;
        }

        constexpr const uint64_t base = 10ULL;
        for(; pos < str.size(); pos++)
        {
            char c = str.at(pos);

            // Check that c is digit and n + digit(c) is within limits
            constexpr const uint64_t uint_max = uint64_t(UINT_MAX);
            if( (c - '0') >= base || n > uint_max / base ||
                n * base > uint_max - (c - '0') ) {
                break;
            }

            // Add digit(c) to result
            n = base * n + (c - '0');
        }

        eosio_assert(( pos == str.size() || (str.at(pos) - '0') >= base ) &&
             n <= uint32_t(INT_MAX) + (neg ? 1U : 0), "Invalid number");

        if(end_pos) *end_pos = pos;
        return static_cast<int32_t>((n^neg) - neg);
    }
}
