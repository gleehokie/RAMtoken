#pragma once
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>

#include "constants.hpp"
#include "fees.hpp"
#include "ds/ram_market.hpp"
#include "../token/eosram.token.hpp"


namespace eosram {
    using namespace eosio;

    struct deducted_amount {
        asset value;
        asset fee;
    };

    template<typename Lambda>
    static deducted_amount deduct_fee(asset amount, Lambda&& fee_f)
    {
        auto fee = fee_f(amount);
        if(fee > amount) 
        {
            fee = amount;
            amount.amount = 0;
        }
        else {
            amount -= fee;
        }

        return { amount, fee };
    }

    static bool is_account_owner_of(eosio::name account, const eosio::extended_symbol& tkn_sym)
    {
        auto& ext_sym = const_cast<eosio::extended_symbol&>(tkn_sym);
        return token::has_balance(
            ext_sym.get_contract(), account, ext_sym.get_symbol().code()
        );
    }

    static asset min_trade_amount(const symbol& sym) 
    {
        if(sym != RAM_SYMBOL)
        {
            ds::ram_market rm;
            return rm.convert_to_eos(asset(min_ram_trade_amount, RAM_SYMBOL));
        } else {
            return asset(min_ram_trade_amount, RAM_SYMBOL);
        }
    }

    static bool is_min_trade_amount(const asset& value)
    {
        return value >= min_trade_amount(value.symbol);
    }

    static void require_min_trade_amount(const asset& value, const char* msg)
    {
        eosio_assert(is_min_trade_amount(value), msg);
    }

    static std::string gen_trade_memo(const asset& sold_amnt, asset price) 
    {
        using namespace std::string_literals;
        const bool is_buy = sold_amnt.symbol == EOS_SYMBOL;
        return ( is_buy ? "Bought"s : "Sold"s) 
            + " RAM tokens @"s + to_string(price) + "/KiB"s;
    }

    static asset min_asset(const asset& a1, const asset& a2)
    {
        eosio_assert(a1.symbol == a2.symbol, "Can't compare amount of different assets");
        return asset(std::min(a1.amount, a2.amount), a1.symbol);
    }
}
