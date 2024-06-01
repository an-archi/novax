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

#ifndef DF548D9F_3819_44FA_89F5_6218193C54F2
#define DF548D9F_3819_44FA_89F5_6218193C54F2
#include <ctp/ThostFtdcMdApi.h>
#include <mutex>
#include <set>

#include "../clock.h"
#include "../data.h"
#include "../definitions.h"
#include "../ns.h"
#include "../proxy.h"
#include "comm.h"

// #define CTP_MD_SETTING_FILE "ctp_md.json"
// #define CTP_MD_SETTING_FILE "/home/ubuntu/code/cub/src/core/ctp/ctp_md.json"
#define CTP_MD_SETTING_FILE "conf/ctp_md.json"
NVX_NS_BEGIN

namespace ctp {
struct mdex : market, CThostFtdcMdSpi {
    mdex( IPub* p_ );

protected:
    NvxSt start() override;
    NvxSt stop() override;
    NvxSt subscribe( const Code& code_ ) override;
    NvxSt unsubscribe( const Code& code_ ) override;

private:
    NvxSt init();
    NvxSt login();
    NvxSt sub();
    NvxSt unsub();
    NvxSt sub( Code& code_ );
    NvxSt unsub( Code& code_ );

private:
    std::unique_ptr<char*[]> set2arr( std::set<Code>& s );

private:
    // ctp overrides
    void OnFrontConnected() override;
    void OnFrontDisconnected( int nReason ) override;
    void OnHeartBeatWarning( int nTimeLapse ) override;
    void OnRspUserLogin( CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRspUserLogout( CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRspQryMulticastInstrument( CThostFtdcMulticastInstrumentField* pMulticastInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRspError( CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRspSubMarketData( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRspUnSubMarketData( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRspSubForQuoteRsp( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRspUnSubForQuoteRsp( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) override;
    void OnRtnDepthMarketData( CThostFtdcDepthMarketDataField* pDepthMarketData ) override;
    void OnRtnForQuoteRsp( CThostFtdcForQuoteRspField* pForQuoteRsp ) override;

private:
    CThostFtdcMdApi* _api = nullptr;
    setting          _settings;
    bool             _is_svc_online = false;

private:
    bool _running = false;

private:
    static id_t session_id();
};
}  // namespace ctp
NVX_NS_END

#endif /* DF548D9F_3819_44FA_89F5_6218193C54F2 */
