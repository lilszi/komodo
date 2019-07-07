/*
 to create a custom libcc.so:

 1. change "func0" and "func1" to method names that fit your custom cc. Of course, you can create more functions by adding another entry to RPC_FUNCS. there is not any practical limit to the number of methods.

 2. For each method make sure there is a UniValue function declaration and CUSTOM_DISPATCH has an if statement checking for it that calls the custom_func

 3. write the actual custom_func0, custom_func1 and custom_validate in customcc.cpp

 4. ./makecustom, which builds cclib.cpp with -DBUILD_CUSTOMCC and puts the libcc.so in ~/komodo/src and rebuilds komodod

 5. launch your chain with -ac_cclib=customcc -ac_cc=2

 */

std::string MYCCLIBNAME = (char *)"customcc";
#include "script/standard.h"

#define EVAL_CUSTOM (EVAL_FAUCET2+1)
#define CUSTOM_TXFEE 10000

#define MYCCNAME "custom"

#define RPC_FUNCS    \
    { (char *)MYCCNAME, (char *)"create", (char *)"<name endtime>", 2, 2, 'C', EVAL_CUSTOM }, \
    { (char *)MYCCNAME, (char *)"bet", (char *)"<amount>", 1, 1, 'B', EVAL_CUSTOM }, \
    { (char *)MYCCNAME, (char *)"status", (char *)"<create_txid>", 1, 1, 'S', EVAL_CUSTOM }, \
    { (char *)MYCCNAME, (char *)"withdraw", (char *)"<create_txid>", 1, 1, 'W', EVAL_CUSTOM }, \
    { (char *)MYCCNAME, (char *)"list", (char *)"<[show complete]>", 0, 1, 'L', EVAL_CUSTOM },

bool custom_validate(struct CCcontract_info *cp,int32_t height,Eval *eval,const CTransaction tx);
UniValue custom_create(uint64_t txfee,struct CCcontract_info *cp,cJSON *params);
UniValue custom_bet(uint64_t txfee,struct CCcontract_info *cp,cJSON *params);
UniValue custom_status(uint64_t txfee,struct CCcontract_info *cp,cJSON *params);
UniValue custom_withdraw(uint64_t txfee,struct CCcontract_info *cp,cJSON *params);
UniValue custom_list(uint64_t txfee,struct CCcontract_info *cp,cJSON *params);

#define CUSTOM_DISPATCH \
if ( cp->evalcode == EVAL_CUSTOM ) \
{ \
    if ( strcmp(method,"create") == 0 ) \
        return(custom_create(txfee,cp,params)); \
    else if ( strcmp(method,"bet") == 0 ) \
        return(custom_bet(txfee,cp,params)); \
    else if ( strcmp(method,"status") == 0 ) \
        return(custom_status(txfee,cp,params)); \
    else if ( strcmp(method,"withdraw") == 0 ) \
        return(custom_withdraw(txfee,cp,params)); \
    else if ( strcmp(method,"list") == 0 ) \
        return(custom_list(txfee,cp,params)); \
    else \
    { \
        result.push_back(Pair("result","error")); \
        result.push_back(Pair("error","invalid customcc method")); \
        result.push_back(Pair("method",method)); \
        return(result); \
    } \
}

template <typename SERIALIZABLE>
std::vector<unsigned char> AsVector(SERIALIZABLE &obj)
{
    CDataStream s = CDataStream(SER_NETWORK, PROTOCOL_VERSION);
    s << obj;
    return std::vector<unsigned char>(s.begin(), s.end());
}

template <typename SERIALIZABLE>
void FromVector(const std::vector<unsigned char> &vch, SERIALIZABLE &obj)
{
    CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);
    obj.Unserialize(s);
}

template <typename SERIALIZABLE>
uint256 GetHash(SERIALIZABLE obj)
{
    CHashWriter hw(SER_GETHASH, PROTOCOL_VERSION);
    hw << obj;
    return hw.GetHash();
}

template <typename T>
bool IsValidObject(const CScript &scr, T &extraObject)
{
    extraObject = T(scr);
    return extraObject.IsValid();
}
/*
    We have funcid in CC params and the object itself, because if someone changes the funcid/version in the cc opt params
    then the object will not be identified correctly and will serialize garbage data, and return as valid but wont be useable.
    By putting funcid last it will overflow and return invalid.
*/
class CCreate
{
public:
    std::string name = "";
    int64_t timestamp;
    uint8_t funcid;

    CCreate() {}
    CCreate(const CScript &);
    CCreate(uint8_t _funcid, std::string _name, uint32_t _timestamp) :
        funcid(_funcid), name(_name), timestamp(_timestamp) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(name);
        READWRITE(timestamp);
        READWRITE(funcid);
    }

    std::vector<unsigned char> AsVector()
    {
        return ::AsVector(*this);
    }

    CCreate(const std::vector<unsigned char> &asVector)
    {
        FromVector(asVector, *this);
    }

    bool IsValid()
    {
        return !name.empty();
    }
};

class CBet
{
public:
    uint256 createtxid = zeroid;
    CAmount satoshis = 0;
    CPubKey payoutpubkey;
    uint8_t funcid;

    CBet() {}
    CBet(const CScript &);
    CBet(uint8_t _funcid, uint256 _createtxid, CAmount _satoshis, CPubKey _payoutpubkey) :
        funcid(_funcid), createtxid(_createtxid), satoshis(_satoshis), payoutpubkey(_payoutpubkey){}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(createtxid);
        READWRITE(satoshis);
        READWRITE(payoutpubkey);
        READWRITE(funcid);
    }

    std::vector<unsigned char> AsVector()
    {
        return ::AsVector(*this);
    }

    CBet(const std::vector<unsigned char> &asVector)
    {
        FromVector(asVector, *this);
    }

    bool IsValid()
    {
        return createtxid != zeroid;
    };
};

class CWithdraw
{
public:
    uint256 createtxid = zeroid;
    uint8_t funcid;
    CPubKey winner;

    CWithdraw() {}
    CWithdraw(const CScript &);
    CWithdraw(uint8_t _funcid, uint256 _createtxid, CPubKey _winner) :
        funcid(_funcid), createtxid(_createtxid), winner(_winner) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(createtxid);
        READWRITE(winner);
        READWRITE(funcid);
    }

    std::vector<unsigned char> AsVector()
    {
        return ::AsVector(*this);
    }

    CWithdraw(const std::vector<unsigned char> &asVector)
    {
        FromVector(asVector, *this);
    }

    bool IsValid()
    {
        return createtxid != zeroid;
    };
};
