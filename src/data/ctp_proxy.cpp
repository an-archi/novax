#include <comm/log.hpp>
#include <filesystem>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>

#include "ctp_proxy.h"

CUB_NS_BEGIN
namespace js = rapidjson;
namespace fs = std::filesystem;

CtpExMd::CtpExMd() {
}

int CtpExMd::subscribue( const code_t& code_ ) {
    std::unique_lock<std::mutex> lock{ _sub_mtx };

    _sub_symbols.insert( code_ );
    _unsub_symbols.erase( code_ );

    if ( _is_svc_online )
        sub( const_cast<code_t&>( code_ ) );

    return 0;
}

int CtpExMd::unsubscribue( const code_t& code_ ) {
    std::unique_lock<std::mutex> lock{ _sub_mtx };

    if ( _sub_symbols.count( code_ ) == 0 ) return 0;

    _sub_symbols.erase( code_ );
    _unsub_symbols.insert( code_ );

    if ( _is_svc_online )
        unsub( const_cast<code_t&>( code_ ) );

    return 0;
}

int CtpExMd::init() {
    if ( read_settings() < 0 ) {
        LOG_INFO( "#ERR,read ctp setings failed" );
        return -1;
    }

    if ( login() < 0 ) {
        LOG_INFO( "log failed" );
        return -2;
    }

    return Market::init();
}

int CtpExMd::sub() {
    if ( _sub_symbols.empty() ) return 0;

    auto arr = set2arr( _sub_symbols );
    return _api->SubscribeMarketData( arr.get(), _sub_symbols.size() );
}

int CtpExMd::unsub() {
    if ( _sub_symbols.empty() ) return 0;

    auto arr = set2arr( _sub_symbols );
    return _api->UnSubscribeMarketData( arr.get(), _unsub_symbols.size() );
}

std::unique_ptr<char*[]> CtpExMd::set2arr( std::set<code_t>& s ) {
    auto arr = std::make_unique<char*[]>( _sub_symbols.size() );
    int  n   = 0;

    for ( auto& s : _unsub_symbols ) {
        arr[ n++ ] = const_cast<code_t&>( s );
    }

    return arr;
}

int CtpExMd::sub( code_t& code_ ) {
    char* c = code_;
    return _api->SubscribeMarketData( &c, 1 );
}

int CtpExMd::unsub( code_t& code_ ) {
    char* c = code_;
    return _api->UnSubscribeMarketData( &c, 1 );
}

int CtpExMd::read_settings() {
    _settings = { 0 };

    std::ifstream json( CTP_MD_SETTING_FILE, std::ios::in );

    if ( !json.is_open() ) {
        LOG_INFO( "open setting failed %s", CTP_MD_SETTING_FILE );
        return -1;
    }

    std::stringstream json_stream;
    json_stream << json.rdbuf();
    std::string setting_str = json_stream.str();

    js::Document d;
    d.Parse( setting_str.data() );

    if ( !d.HasMember( "proxies" ) || !d[ "proxies" ].IsArray() ) {
        LOG_INFO( "setting foramt error ,'proxies' node is not array" );
        return -2;
    }

    auto proxies = d[ "proxies" ].GetArray();
    //! 找到第一个enable的就结束
    for ( auto& p : proxies ) {
        if ( p.HasMember( "enabled" ) || ( p.HasMember( "enabled" ) && !d.GetBool() ) ) {
            continue;
        }

        _settings.flow_path      = p.HasMember( "flow_path" ) ? p.GetString() : ".";
        _settings.conn.broker    = p.HasMember( "broker" ) ? p.GetString() : "uknown";
        _settings.conn.password  = p.HasMember( "password" ) ? p.GetString() : "";
        _settings.conn.user_name = p.HasMember( "user_name" ) ? p.GetString() : "";
        _settings.conn.frontend  = p.HasMember( "frontend" ) ? p.GetString() : "";
        break;
    }
    return 0;
}

id_t CtpExMd::session_id() {
    static id_t _rid = 0;
    return ++_rid;
}

int CtpExMd::login() {
    CThostFtdcReqUserLoginField login_req = { 0 };

    memcpy( login_req.BrokerID, _settings.conn.broker.c_str(), std::min( _settings.conn.broker.length(), sizeof( login_req.BrokerID ) - 1 ) );
    memcpy( login_req.UserID, _settings.conn.user_name.c_str(), std::min( _settings.conn.user_name.length(), sizeof( login_req.UserID ) - 1 ) );
    memcpy( login_req.Password, _settings.conn.password.c_str(), std::min( _settings.conn.password.length(), sizeof( login_req.Password ) - 1 ) );

    auto session = session_id();

    int rt = this->_api->ReqUserLogin( &login_req, session );
    if ( !rt ) {
        LOG_INFO( "i> LqSvcMarket::xLogin sucessfully, sess=%u\n", ( unsigned )session );
        return 0;
    }
    else {
        LOG_INFO( "c> LqSvcMarket::xLogin failed\n" );
        return -1;
    }

    return 0;
}

int CtpExMd::init() {
    LOG_INFO( "ctp md init,flow=%s,font=%s", _settings.flow_path.c_str(), _settings.conn.frontend.c_str() );

    _api = CThostFtdcMdApi::CreateFtdcMdApi( _settings.flow_path.c_str(), false );  // true: udp mode
    _api->RegisterSpi( this );
    _api->RegisterFront( const_cast<char*>( _settings.conn.frontend.c_str() ) );
    _api->Init();
    _api->Join();

    return 0;
}

// ctp overrides
void CtpExMd::OnFrontConnected() {
    LOG_INFO( "ctp ex md connected" );
    login();
}

void CtpExMd::OnFrontDisconnected( int nReason ) {
    LOG_INFO( "ctp ex md disconnected,reason=%d", nReason );

    _is_svc_online = false;
}

//! as doc: this is decrecated
void CtpExMd::OnHeartBeatWarning( int nTimeLapse ) {
}

#define LOG_ERROR_AND_RET( _rsp_, _req_, _lst_ )                                                                            \
    do {                                                                                                                    \
        if ( pRspInfo->ErrorID != 0 ) {                                                                                     \
            LOG_INFO( "usr logout error: code=%d msg=%s, req=%d, last=%d", _rsp_->ErrorID, _rsp_->ErrorMsg, _req_, _lst_ ); \
            return;                                                                                                         \
        }                                                                                                                   \
    } while ( 0 )

// #这里有问题，如果重连的时候是否需要重新订阅
void CtpExMd::OnRspUserLogin( CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_ERROR_AND_RET( pRspInfo, nRequestID, bIsLast );

    LOG_INFO( "@INFO, login:day=%s, time=%s,brokerid=%s, user=%s, sysname=%s,frontid=%s,session=%s, order_ref=%d",
              pRspUserLogin->TradingDay,
              pRspUserLogin->LoginTime,
              pRspUserLogin->BrokerID,
              pRspUserLogin->UserID,
              pRspUserLogin->SystemName,
              pRspUserLogin->FrontID,
              pRspUserLogin->SessionID,
              pRspUserLogin->MaxOrderRef );

    _clock[ ( int )extype_t::SHFE ].tune( pRspUserLogin->SHFETime );
    _clock[ ( int )extype_t::DCE ].tune( pRspUserLogin->DCETime );
    _clock[ ( int )extype_t::CZCE ].tune( pRspUserLogin->CZCETime );
    _clock[ ( int )extype_t::FFEX ].tune( pRspUserLogin->FFEXTime );
    _clock[ ( int )extype_t::INE ].tune( pRspUserLogin->INETime );
    _clock[ ( int )extype_t::GFEX ].tune( pRspUserLogin->GFEXTime );

    {
        std::unique_lock<std::mutex> lock{ _sub_mtx };
        unsub();
        sub();

        _is_svc_online = true;
    }
}

void CtpExMd::OnRspUserLogout( CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_ERROR_AND_RET( pRspInfo, nRequestID, bIsLast );

    LOG_INFO( "user logout: broker=%s ,user=%s, req=%d", pUserLogout->BrokerID, pUserLogout->UserID, nRequestID );
    _is_svc_online = false;
}

void CtpExMd::cvt_datetime( DateTime&                     dt,
                            const TThostFtdcDateType&     ctp_day_,
                            const TThostFtdcTimeType&     ctp_time_,
                            const TThostFtdcMillisecType& ctp_milli_ ) {
    assert( 0 );
}

void CtpExMd::OnRtnDepthMarketData( CThostFtdcDepthMarketDataField* f ) {
    quotation_t q{ 0 };

    cvt_datetime( q.time, f->TradingDay, f->UpdateTime, f->UpdateMillisec );  // todo use ActionDay?

    q.last        = f->LastPrice;
    q.volume      = f->Volume;
    q.code        = f->InstrumentID;
    q.opi         = f->OpenInterest;
    q.depth       = 1;
    q.ask[ 0 ]    = f->AskPrice1;
    q.askvol[ 0 ] = f->AskVolume1;
    q.bid[ 0 ]    = f->BidPrice1;
    q.bidvol[ 0 ] = f->BidVolume1;
    q.highest     = f->HighestPrice;
    q.lowest      = f->LowestPrice;
    q.avgprice    = f->AveragePrice;
    q.amount      = f->Turnover;
    q.open        = f->OpenPrice;
    q.close       = f->ClosePrice;

    /*
    鉴于夜盘交易时间非常混乱，我们不使用服务器时间（日期），和常用的交易软件时间划分类似
    夜盘算一整天的开始，所以只要时间在下午3点之后都算第二天，9点之后算当天

    但是。。。这会有节假日的问题，这会极大的增加问题的复杂度，我们可以记录两个值来表示日期
    当前日期和偏移量，晚上偏移量是1，白天偏移量是0，但是。。。。新的问题来了，本来这个只是为了数据存储使用（一天一个文件夹）

    实际上这个偏移量也不用记，根据时间推算就可以了--周末和节假日怎么办
    文华财经是按自然日来做的，9点到夜里收盘算是一天

    https://zhuanlan.zhihu.com/p/33553653
    总之（敲黑板），处理上期所和大商所的夜盘品种，可以直接用TradingDay取一个完整的交易日，而处理郑商所的夜盘品种，则需要拼接上一个TradingDay的夜盘和当前TradingDay的日盘作为一个完整的交易日。
    */

    // this->proxy->OnStreaming( f->InstrumentID, t );
}

void CtpExMd::OnRspQryMulticastInstrument( CThostFtdcMulticastInstrumentField* pMulticastInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_ERROR_AND_RET( pRspInfo, nRequestID, bIsLast );
    LOG_INFO( "multicast return" );
}

void CtpExMd::OnRspError( CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_INFO( "rsp error,code=%d, msg=%s", pRspInfo->ErrorID, pRspInfo->ErrorMsg );
}

void CtpExMd::OnRspSubMarketData( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_ERROR_AND_RET( pRspInfo, nRequestID, bIsLast );

    LOG_INFO( "subscribe return: %s", pSpecificInstrument->InstrumentID );
}

void CtpExMd::OnRspUnSubMarketData( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_ERROR_AND_RET( pRspInfo, nRequestID, bIsLast );

    LOG_INFO( "subscribe return: %s", pSpecificInstrument->InstrumentID );
}

void CtpExMd::OnRspSubForQuoteRsp( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_ERROR_AND_RET( pRspInfo, nRequestID, bIsLast );
    LOG_INFO( "rsp for quote" );
}

void CtpExMd::OnRtnForQuoteRsp( CThostFtdcForQuoteRspField* pForQuoteRsp ) {
    LOG_INFO( "QUOTE RSP" );
}

void CtpExMd::OnRspUnSubForQuoteRsp( CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast ) {
    LOG_ERROR_AND_RET( pRspInfo, nRequestID, bIsLast );
    LOG_INFO( "rsp for unsubquote" );
}

CUB_NS_END