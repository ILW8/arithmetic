
#include <cctype>
#include <stdlib.h>
#include "gwnum.h"
#include "cpuid.h"
#include "inputnum.h"
#include "edwards.h"

using namespace arithmetic;

bool InputNum::parse(const std::string& s)
{
    std::string::const_iterator it, it_s;

    for (it = s.begin(); it != s.end() && std::isspace(*it); it++);
    if (it != s.end() && *it == '\"')
        it++;
    for (it_s = it; it != s.end() && std::isdigit(*it); it++);
    if (it == it_s)
        return false;
    if (it != s.end() && *it == '*')
    {
        _gk = std::string(it_s, it);
        for (it_s = ++it; it != s.end() && std::isdigit(*it); it++);
        if (it == it_s)
            return false;
    }
    else
        _gk = 1;
    if (it == s.end() || *it == '^' || *it == '\"')
    {
        _gb = std::string(it_s, it);
    }
    else
        return false;
    if (it != s.end() && *it == '^')
    {
        for (it_s = ++it; it != s.end() && std::isdigit(*it); it++);
        if (it == it_s || it - it_s > 9)
            return false;
        _n = stoi(std::string(it_s, it));
        if (it == s.end())
            return false;
        else if (*it == '+')
            _c = 1;
        else if (*it == '-')
            _c = -1;
        else
            return false;
        for (it_s = ++it; it != s.end() && std::isdigit(*it); it++);
        if (it == it_s || it - it_s > 9)
            return false;
        _c *= stoi(std::string(it_s, it));
    }
    else
    {
        _n = 1;
        _c = 0;
    }
    if (it != s.end() && *it == '\"')
        it++;
    for (; it != s.end() && std::isspace(*it); it++);
    if (it != s.end())
        return false;

    process();
    return true;
}

void InputNum::add_factor(uint32_t factor, int power)
{
    Giant tmp;
    tmp = factor;
    add_factor(tmp, power);
}

void InputNum::add_factor(const Giant& factor, int power)
{
    auto it = _b_factors.begin();
    for (; it != _b_factors.end() && it->first != factor; it++);
    if (it != _b_factors.end())
        it->second += power;
    else
        _b_factors.emplace_back(factor, power);
}

void InputNum::process()
{
    _b_factors.clear();
    _b_cofactor.reset();

    uint32_t i, j;
    Giant tmp = _gb;
    for (i = 0; !tmp.bit(i); i++);
    if (i > 0)
    {
        add_factor(2, i);
        tmp >>= i;
    }
    uint32_t s = 10;
    std::vector<char> bitmap;
    if (tmp > 1)
    {
        bitmap.resize((size_t)1 << (s - 1), 0);
        std::vector<std::pair<int, int>> smallprimes;
        for (i = 1; i < bitmap.size(); i++)
            if (!bitmap[i])
            {
                smallprimes.emplace_back(i*2 + 1, (i*2 + 1)*(i*2 + 1)/2);
                if (i < ((size_t)1 << (s/2 - 1)))
                    for (; smallprimes.back().second < bitmap.size(); smallprimes.back().second += smallprimes.back().first)
                        bitmap[smallprimes.back().second] = 1;
                for (; tmp%(i*2 + 1) == 0; tmp /= i*2 + 1)
                    add_factor(i*2 + 1);
            }
        if (tmp > 1 && tmp < (1 << (2*s)))
        {
            add_factor(tmp);
            tmp = 1;
        }
        for (j = 0; j < s && tmp > 1; j += 5)
        {
            bitmap.resize((size_t)1 << (s - 1 + j + 5), 0);
            for (auto it = smallprimes.begin(); it != smallprimes.end(); it++)
                for (; it->second < bitmap.size(); it->second += it->first)
                    bitmap[it->second] = 1;
            for (i = 1 << (s - 1 + j); i < bitmap.size(); i++)
                for (; !bitmap[i] && tmp%(i*2 + 1) == 0; tmp /= i*2 + 1)
                    add_factor(i*2 + 1);
            if (tmp > 1 && (uint32_t)tmp.bitlen() <= 2*(s + j + 4))
            {
                add_factor(tmp);
                tmp = 1;
            }
        }
    }
    if (tmp > 1)
        _b_cofactor.reset(new Giant(tmp));

    _input_text = build_text();

/*    if (_b_cofactor)
    {
        int len;
        GWState gwstate;
        gwstate.setup(*_b_cofactor);
        GWArithmetic gw(gwstate);
        {
            GWNum y(gw);
            y = 3;
            gw.setmulbyconst(3);
            len = _b_cofactor->bitlen() - 1;
            for (i = 1; i <= len; i++)
                gw.mul(y, y, y, _b_cofactor->bit(len - i) ? GWMUL_MULBYCONST : 0);
            if (y == 3)
            {
                add_factor(*_b_cofactor);
                _b_cofactor.reset();
            }
        }
    }

    if (_b_cofactor)
    {
        int len = _b_cofactor->bitlen()/10;
        if (len > 2*s)
            len = 2*s;
        tmp.arithmetic().alloc(tmp, (1 << (len - 5))/0.69);
        Giant tmp2(tmp.arithmetic(), tmp.capacity() < 8192 ? tmp.capacity() : 8192);
        tmp = 0;
        tmp2 = 1 << len;
        for (i = 1; i < bitmap.size() && i < (1 << len); i++)
            if (!bitmap[i])
            {
                uint32_t p = i*2 + 1;
                j = p;
                if (p <= (1 << (len/2 + 1)))
                {
                    uint32_t pp = (1 << len)/p;
                    while (j <= pp)
                        j *= p;
                }
                tmp2 *= j;
                if (tmp2.size() > 8190)
                {
                    if (tmp == 0)
                        tmp = tmp2;
                    else
                        tmp *= tmp2;
                    tmp2 = 1;
                }
            }
        if (tmp == 0)
            tmp = std::move(tmp2);
        else
            tmp *= tmp2;
        len = tmp.bitlen();
        int W;
        for (W = 2; W < 16 && (14 << (W - 2)) + len/0.69*(7 + 7/(W + 1.0)) >(14 << (W - 1)) + len/0.69*(7 + 7/(W + 2.0)); W++);
        std::vector<int16_t> naf_w;
        get_NAF_W(W, tmp, naf_w);
        GWState gwstate;
        gwstate.setup(*_b_cofactor);
        GWArithmetic gw(gwstate);
        for (j = 0; true; j++)
        {
            double timer = getHighResTimer();
            {
                EdwardsArithmetic ed(gw);
                GWNum ed_d(gw);
                EdPoint P = ed.gen_curve(*(int *)&timer, &ed_d);
                ed.mul(P, W, naf_w, P);
                if (!ed.on_curve(P, ed_d))
                {
                    gwstate.done();
                    gwstate.next_fft_count++;
                    gwstate.setup(*_b_cofactor);
                }
                tmp = gcd(*P.X, *_b_cofactor);
            }
            if (tmp != 1)
            {
                GWState rgwstate;
                rgwstate.setup(tmp);
                GWArithmetic rgw(rgwstate);
                GWNum x(rgw);
                x = 3;
                rgw.setmulbyconst(3);
                len = tmp.bitlen() - 1;
                for (i = 1; i <= len; i++)
                    rgw.mul(x, x, x, tmp.bit(len - i) ? GWMUL_MULBYCONST : 0);
                if (x == 3)
                {
                    add_factor(tmp);
                    *_b_cofactor /= tmp;
                    gwstate.done();
                    gwstate.setup(*_b_cofactor);
                    GWNum y(gw);
                    y = 3;
                    gw.setmulbyconst(3);
                    len = _b_cofactor->bitlen() - 1;
                    for (i = 1; i <= len; i++)
                        gw.mul(y, y, y, _b_cofactor->bit(len - i) ? GWMUL_MULBYCONST : 0);
                    if (y == 3)
                    {
                        add_factor(*_b_cofactor);
                        _b_cofactor.reset();
                        break;
                    }
                }
            }
        }
    }*/

    if (_gb != 1)
    {
        if (!_b_cofactor)
        {
            int a = _b_factors[0].second;
            for (auto it = ++_b_factors.begin(); it != _b_factors.end(); it++)
                a = gcd(a, it->second);
            if (a > 1)
            {
                for (auto it = _b_factors.begin(); it != _b_factors.end(); it++)
                    it->second /= a;
                _n *= a;
                _gb = 1;
                for (auto it = _b_factors.begin(); it != _b_factors.end(); it++)
                    _gb *= power(it->first, it->second);
            }
        }

        while (_gk%_gb == 0)
        {
            _gk /= _gb;
            _n++;
        }
    }

    if (_gk == 1 && _c == 1 && _n > 1 && (_n & (_n - 1)) == 0)
        for (_gfn = 1; (1UL << _gfn) < _n; _gfn++);

    _display_text = build_text(40);
}

std::string InputNum::build_text(int max_len)
{
    std::string res;
    res.reserve(32);
    if (_gk != 1)
    {
        std::string sk = _gk.to_string();
        if (sk.size() > max_len && max_len > 0)
        {
            res.append(sk, 0, max_len/2);
            res.append(3, '.');
            res.append(sk, sk.size() - max_len/2, max_len/2);
        }
        else
            res.append(sk);
        if (_gb != 1)
            res.append(1, '*');
    }
    if (_gb != 1)
    {
        std::string sb = _gb.to_string();
        if (sb.size() > max_len && max_len > 0)
        {
            res.append(sb, 0, max_len/2);
            res.append(3, '.');
            res.append(sb, sb.size() - max_len/2, max_len/2);
        }
        else
            res.append(sb);
        if (_n != 1)
        {
            res.append(1, '^');
            res.append(std::to_string(_n));
        }
    }
    if (_c != 0)
    {
        if (_c > 0)
            res.append(1, '+');
        if (_c < 0)
            res.append(1, '-');
        res.append(std::to_string(abs(_c)));
    }

    return res;
}

void InputNum::setup(GWState& state)
{
    if (k() != 0 && b() != 0)
    {
        state.setup(k(), b(), _n, _c);
    }
    else
    {
        state.setup(_gk*power(_gb, _n) + _c);
    }
}

bool InputNum::to_base2(InputNum& k, InputNum& base2)
{
    if (_b_factors.size() == 0 || _b_factors[0].first != 2)
        return false;
    if (_gb.bitlen() > _b_factors[0].second*2)
        return false;
    int num2 = _b_factors[0].second;
    int n2 = _n*num2;
    int c = _c;

    k._gk = _gk;
    k._gb = _gb >> num2;
    k._n = _n;
    k._c = 0;
    k.process();

    base2._gk = k._gk*power(k._gb, k._n);
    base2._gb = 2;
    base2._n = n2;
    base2._c = c;
    base2._b_factors.clear();
    base2.add_factor(2);
    base2._b_cofactor.reset();
    base2._input_text = _input_text;
    base2._display_text = k._display_text;
    base2._display_text.append(1, '*');
    base2._display_text.append(1, '2');
    base2._display_text.append(1, '^');
    base2._display_text.append(std::to_string(n2));
    if (c > 0)
        base2._display_text.append(1, '+');
    if (c < 0)
        base2._display_text.append(1, '-');
    if (c != 0)
        base2._display_text.append(std::to_string(abs(c)));

    return true;
}