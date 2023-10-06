#ifndef B43732C7_EA9D_4138_8023_E0627CD66A48
#define B43732C7_EA9D_4138_8023_E0627CD66A48
#include <algorithm>
#include <atomic>
#include <math.h>
#include <unordered_map>

#include "../definitions.h"
#include "../ns.h"

//--! 一个req要么
CUB_NS_BEGIN

namespace ctp {
#define LOG_ERROR_AND_RET( _rsp_, _req_, _lst_ )                                                                            \
    do {                                                                                                                    \
        if ( pRspInfo->ErrorID != 0 ) {                                                                                     \
            LOG_INFO( "usr logout error: code=%d msg=%s, req=%d, last=%d", _rsp_->ErrorID, _rsp_->ErrorMsg, _req_, _lst_ ); \
            return;                                                                                                         \
        }                                                                                                                   \
    } while ( 0 )

#define CTP_COPY_SAFE( _field_, _str_ ) memcpy( _field_, _str_, std::min( ( int )strlen( _str_ ), ( int )sizeof( _field_ ) - 1 ) )

struct cert_t {
    string_t auth;
    string_t appid;
    string_t token;
};

struct investor_t {
    string_t broker;
    string_t frontend;
    string_t name;
    string_t password;
};

struct setting_t {
    string_t   flow_path;
    investor_t i;
    cert_t     c;
};

enum class extype_t : uint8_t {
    SHFE = 0,
    DCE,
    CZCE,
    FFEX,
    INE,
    GFEX,

    Count
};

enum class act_t : uint8_t {
    login,
    logout,
    sub,
    unsub,
    auth,

    put_order,
    cancel_order,
    qry_settle,
    qry_commission,
    qry_fund,
    qry_position
};

/*重复也不太有关系*/
inline int req_id() {
    static std::atomic<int> r = 0;
    return r++;
}

using req_map_t = std::unordered_map<act_t, int>;

inline void cvt_datetime( datetime_t&                   dt,
                          const TThostFtdcDateType&     ctp_day_,
                          const TThostFtdcTimeType&     ctp_time_,
                          const TThostFtdcMillisecType& ctp_milli_ );
}  // namespace ctp
CUB_NS_END
#endif /* B43732C7_EA9D_4138_8023_E0627CD66A48 */