/*
 to create a custom libcc.so:
 
 1. change "func0" and "func1" to method names that fit your custom cc. Of course, you can create more functions by adding another entry to RPC_FUNCS. there is not any practical limit to the number of methods.
 
 2. For each method make sure there is a UniValue function declaration and CUSTOM_DISPATCH has an if statement checking for it that calls the custom_func
 
 3. write the actual custom_func0, custom_func1 and custom_validate in customcc.cpp
 
 4. ./makecustom, which builds cclib.cpp with -DBUILD_CUSTOMCC and puts the libcc.so in ~/komodo/src and rebuilds komodod
 
 5. launch your chain with -ac_cclib=customcc -ac_cc=2
 
 */
 
std::string MYCCLIBNAME = (char *)"customcc";

#define EVAL_CUSTOM (EVAL_FAUCET2+1)
#define CUSTOM_TXFEE 10000

#define MYCCNAME "custom"

#define RPC_FUNCS    \
    { (char *)MYCCNAME, (char *)"func0", (char *)"<parameter help>", 1, 1, '0', EVAL_CUSTOM }, \
    { (char *)MYCCNAME, (char *)"func1", (char *)"<no args>", 0, 0, '1', EVAL_CUSTOM },

bool custom_validate(struct CCcontract_info *cp,int32_t height,Eval *eval,const CTransaction tx);
UniValue custom_func0(uint64_t txfee,struct CCcontract_info *cp,cJSON *params);
UniValue custom_func1(uint64_t txfee,struct CCcontract_info *cp,cJSON *params);

#define CUSTOM_DISPATCH \
if ( cp->evalcode == EVAL_CUSTOM ) \
{ \
    if ( strcmp(method,"func0") == 0 ) \
        return(custom_func0(txfee,cp,params)); \
    else if ( strcmp(method,"func1") == 0 ) \
        return(custom_func1(txfee,cp,params)); \
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

class CGameCreate
{
public:
    uint8_t nVersion = 1;
    std::string name = "";
    CPubKey playerpk;
    
    CGameCreate() {}
    CGameCreate(const CTxOut &);
    CGameCreate(std::string _name, CPubKey _playerpk) :
        name(_name), playerpk(_playerpk) {}
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nVersion);
        READWRITE(name);
        READWRITE(playerpk);
    }

    std::vector<unsigned char> AsVector()
    {
        return ::AsVector(*this);
    }

    bool IsValid()
    {
        return !name.empty();
    }
};

class CEvents
{
public:
    uint8_t nVersion = 1;
    uint32_t counter;
    uint256 gametxid = zeroid;
    std::vector<uint8_t> events;
    
    CEvents() {}
    CEvents(const CTxOut &);
    CEvents(uint32_t _counter, uint256 _gametxid, std::vector<uint8_t> _events) :
        counter(_counter), gametxid(_gametxid), events(_events) {}
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nVersion);
        READWRITE(counter);
        READWRITE(gametxid);
        READWRITE(events);
    }

    std::vector<unsigned char> AsVector()
    {
        return ::AsVector(*this);
    }

    bool IsValid()
    {
        return gametxid != zeroid;
    };
};
