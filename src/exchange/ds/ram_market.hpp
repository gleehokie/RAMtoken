#pragma once
#include <eosiolib/eosio.hpp>
#include <tuple>
#include "ram_exchange_state.hpp"
#include "../constants.hpp"

namespace eosram::ds {
    using eosio::asset;

    /* Struct defines kibibyte unit */
    struct kibyte
    {
        constexpr explicit kibyte(uint64_t v) : value(v)
        {
            eosio_assert(v <= UINT32_MAX / 1024UL, "kibyte: Too big value for KiB unit!");
        }

        uint32_t value = 0;
        uint32_t to_bytes() const
        {
            return value * 1024UL;
        }
    };

    // kibyte literal
    constexpr kibyte operator"" _KiB (unsigned long long value)
    {
        return kibyte(value);
    }


    class ram_market
    {
    public:
        ram_market() :
            m_(k_eosio, k_eosio)
        {}

        const eosiosystem::exchange_state& get_state() const
        {
            constexpr auto rc_sym = S(4, RAMCORE);
            auto it = m_.find(rc_sym);
            eosio_assert(it != m_.end(), "ram_market: Could not find eosiosystem rammarket!");
            return *it;
        }

        /** Rerurns RAM price per KiB in EOS */
        asset get_ramprice() const
        {
            const auto& s = get_state();
            auto ramprice = s.quote.balance * 1024; // Calculate balance for 1 KiB
            ramprice /= s.base.balance.amount;

            eosio_assert(ramprice.is_valid(), "ram_market::get_ramprice: Could not calculate valid RAM price!");
            return ramprice;
        }

        asset convert_to_eos(asset from_ram) const
        {
            auto tmp = get_state();
            return tmp.convert(from_ram, CORE_SYMBOL);
        }

        asset convert_to_ram(asset from_eos) const
        {
            auto tmp = get_state();
            return tmp.convert(from_eos, RAM_SYMBOL);
        }

        static void buyram(account_name buyer, account_name receiver, asset eos_quantity)
        {
            constexpr auto k_buyram = "buyram"_n;
            eosio::dispatch_inline(k_eosio, k_buyram, {{ buyer, k_active }}, 
                std::make_tuple(buyer, receiver, eos_quantity)
            );
        }

        static void buyrambytes(account_name buyer, account_name receiver, uint32_t bytes)
        {
            constexpr auto k_buyrambytes = "buyrambytes"_n;
            eosio::dispatch_inline(k_eosio, k_buyrambytes, {{buyer, k_active }}, 
                std::make_tuple(buyer, receiver, bytes)
            );
        }

        static void sellram(account_name seller, kibyte quantity)
        {
            sellrambytes(seller, quantity.to_bytes());
        }

        static void sellrambytes(account_name seller, uint32_t bytes)
        {
            constexpr auto k_sellram = "sellram"_n;
            eosio::dispatch_inline(k_eosio, k_sellram, {{ seller, k_active }}, 
                std::make_tuple(seller, static_cast<int64_t>(bytes))
            );
        }

    private:
        eosiosystem::rammarket m_;
    };
}