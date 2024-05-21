/************************************************************************************
The MIT License

Copyright (c) 2024 YaoZinan  [zinan@outlook.com, nvx-quant.com]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

* \author: yaozn(zinan@outlook.com)
* \date: 2024
**********************************************************************************/

#ifndef C2E26F98_58D2_4FB6_9B05_CB4ED59A65C3
#define C2E26F98_58D2_4FB6_9B05_CB4ED59A65C3

#include "definitions.h"
#include "models.h"

NVX_NS_BEGIN
struct Aspect;
struct IPosition;

struct IContext {
    static IContext* create();

    virtual const quotation_t& qut() const        = 0;
    virtual const fund_t       fund() const       = 0;
    virtual Aspect*            load( const code_t&   symbol_,
                                     const period_t& period_,
                                     int             count_ ) = 0;

    virtual oid_t open( const code_t& c_,
                        vol_t         qty_,
                        price_t       sl_    = 0,
                        price_t       tp_    = 0,
                        price_t       price_ = 0,
                        otype_t       mode_  = otype_t::market ) = 0;

    virtual nvx_st     close( const code_t& c_,
                              vol_t         qty_,
                              price_t       price_ = 0,
                              otype_t       mode_  = otype_t::market ) = 0;
    virtual IPosition* qry_long( const code_t& c_ )             = 0;
    virtual IPosition* qry_short( const code_t& c_ )            = 0;
    virtual datetime_t time() const                             = 0;
    virtual nvxerr_t   error() const                            = 0;
    virtual ~IContext();
};

NVX_NS_END

#endif /* C2E26F98_58D2_4FB6_9B05_CB4ED59A65C3 */
