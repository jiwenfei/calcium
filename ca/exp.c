/*
    Copyright (C) 2020 Fredrik Johansson

    This file is part of Calcium.

    Calcium is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "ca.h"

int ca_as_fmpq_pi_i(fmpq_t res, const ca_t x, ca_ctx_t ctx)
{
    ca_field_ptr K;
    ca_t t;
    int found;

    if (CA_IS_SPECIAL(x))
        return 0;

    K = CA_FIELD(x, ctx);

    if (CA_FIELD_IS_QQ(K) || CA_FIELD_IS_NF(K))
        return 0;

    ca_init(t, ctx);
    ca_pi_i(t, ctx);
    ca_div(t, x, t, ctx);

    if (CA_IS_QQ(t, ctx))
    {
        fmpq_set(res, CA_FMPQ(t));
        found = 1;
    }
    else
    {
        found = 0;
    }

    ca_clear(t, ctx);

    return found;
}

ca_ext_ptr
ca_is_fmpq_times_gen_as_ext(fmpq_t c, const ca_t x, ca_ctx_t ctx)
{
    ca_field_ptr K;

    if (CA_IS_SPECIAL(x))
        return NULL;

    K = CA_FIELD(x, ctx);

    if (CA_FIELD_IS_QQ(K))
        return NULL;

    /* todo */
    if (CA_FIELD_IS_NF(K))
        return NULL;

    if (fmpz_mpoly_is_fmpz(fmpz_mpoly_q_denref(CA_MPOLY_Q(x)), CA_FIELD_MCTX(K, ctx)))
    {
        if (fmpz_mpoly_length(fmpz_mpoly_q_numref(CA_MPOLY_Q(x)), CA_FIELD_MCTX(K, ctx)) == 1)
        {
            fmpz_mpoly_q_t t;
            fmpz_t one;

            /* hack! */
            *t = *CA_MPOLY_Q(x);
            *one = 1;

            fmpz_mpoly_q_numref(t)->coeffs = one;
            fmpz_mpoly_q_denref(t)->coeffs = one;

            if (fmpz_mpoly_is_gen(fmpz_mpoly_q_numref(t), -1, CA_FIELD_MCTX(K, ctx)))
            {
                slong i;

                fmpz_set(fmpq_numref(c), fmpz_mpoly_q_numref(CA_MPOLY_Q(x))->coeffs);
                fmpz_set(fmpq_denref(c), fmpz_mpoly_q_denref(CA_MPOLY_Q(x))->coeffs);

                for (i = 0; ; i++)
                    if (fmpz_mpoly_is_gen(fmpz_mpoly_q_numref(t), i, CA_FIELD_MCTX(K, ctx)))
                        return CA_FIELD_EXT_ELEM(K, i);
            }
        }
    }

    return NULL;
}

void
ca_exp(ca_t res, const ca_t x, ca_ctx_t ctx)
{
    ca_ext_ptr ext;

    if (CA_IS_SPECIAL(x))
    {
        /* todo: complex signed infinity -> 0, undefined, uinf */

        if (ca_check_is_pos_inf(x, ctx) == T_TRUE)
            ca_pos_inf(res, ctx);
        else if (ca_check_is_neg_inf(x, ctx) == T_TRUE)
            ca_zero(res, ctx);
        else if (ca_check_is_undefined(x, ctx) == T_TRUE || ca_check_is_uinf(x, ctx) == T_TRUE)
            ca_undefined(res, ctx);
        else
            ca_unknown(res, ctx);
        return;
    }

    ext = ca_is_gen_as_ext(x, ctx);

    /* exp(log(z)) = z */
    if (ext != NULL && CA_EXT_HEAD(ext) == CA_Log)
    {
        ca_set(res, CA_EXT_FUNC_ARGS(ext), ctx);
        return;
    }

    /* exp((p/q)*log(z)) = z^(p/q) */
    /* todo: when to rewrite any exp(a*log(b)) -> b^a?  */
    {
        fmpq_t t;
        fmpq_init(t);
        ext = ca_is_fmpq_times_gen_as_ext(t, x, ctx);
        if (ext != NULL && CA_EXT_HEAD(ext) == CA_Log)
        {
            ca_pow_fmpq(res, CA_EXT_FUNC_ARGS(ext), t, ctx);
            fmpq_clear(t);
            return;
        }
        fmpq_clear(t);
    }

    if (ca_check_is_zero(x, ctx) == T_TRUE)
    {
        ca_one(res, ctx);
        return;
    }

    /* more generally, exp(p/q*pi*i) -> root of unity */
    {
        fmpq_t t;
        fmpq_init(t);
        if (ca_as_fmpq_pi_i(t, x, ctx))
        {
            if (fmpz_cmp_ui(fmpq_denref(t), 12) <= 0)
            {
                slong p, q;
                qqbar_t a;

                q = fmpz_get_ui(fmpq_denref(t));
                p = fmpz_fdiv_ui(fmpq_numref(t), 2 * q);

                if (q == 1)
                {
                    if (p == 0)
                        ca_one(res, ctx);
                    else
                        ca_neg_one(res, ctx);
                }
                else if (q == 2)
                {
                    if (p == 1)
                        ca_i(res, ctx);
                    else
                        ca_neg_i(res, ctx);
                }
                else
                {
                    qqbar_init(a);
                    qqbar_exp_pi_i(a, 1, q);
                    ca_set_qqbar(res, a, ctx);
                    ca_pow_ui(res, res, p, ctx);
                    qqbar_clear(a);
                }

                fmpq_clear(t);
                return;
            }
        }
        fmpq_clear(t);
    }

    _ca_make_field_element(res, _ca_ctx_get_field_fx(ctx, CA_Exp, x), ctx);
    fmpz_mpoly_q_gen(CA_MPOLY_Q(res), 0, CA_MCTX_1(ctx));
    /* todo: detect simple values instead of creating extension element in the first place */
    _ca_mpoly_q_reduce_ideal(CA_MPOLY_Q(res), CA_FIELD(res, ctx), ctx);
    ca_condense_field(res, ctx);
}

