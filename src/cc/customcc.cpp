 /********************************************************************
  * (C) 2019 Michael Toutonghi
  * 
  * Distributed under the MIT software license, see the accompanying
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.
  * 
  * 
  *  ported by blackjok3r
  */

 template <typename TOBJ>
 CTxOut MakeCC1of1Vout(uint8_t funcid, uint8_t evalcode, CAmount nValue, CPubKey pk, TOBJ &obj)
 {
     CTxOut vout;
     CC *payoutCond = MakeCCcond1(evalcode, pk);
     vout = CTxOut(nValue, CCPubKey(payoutCond));
     cc_free(payoutCond);
     std::vector<CPubKey> vpk({pk});
     std::vector<std::vector<unsigned char>> vvch({::AsVector(obj)});
     
     COptCCParams vParams = COptCCParams(funcid, evalcode, 1, 1, vpk, vvch);
     // add the object to the end of the script
     vout.scriptPubKey << vParams.AsVector() << OP_DROP;
     return(vout);
 }

 template <typename TOBJ>
 CTxOut MakeCC1of2Vout(uint8_t funcid, uint8_t evalcode, CAmount nValue, CPubKey pk1, CPubKey pk2, TOBJ &obj)
 {
     CTxOut vout;
     CC *payoutCond = MakeCCcond1of2(evalcode, pk1, pk2);
     vout = CTxOut(nValue,CCPubKey(payoutCond));
     cc_free(payoutCond);

     std::vector<CPubKey> vpk({pk1, pk2});
     std::vector<std::vector<unsigned char>> vvch({::AsVector(obj)});
     COptCCParams vParams = COptCCParams(funcid, evalcode, 1, 2, vpk, vvch);

     // add the object to the end of the script
     vout.scriptPubKey << vParams.AsVector() << OP_DROP;
     return(vout);
 }
 // --------------------------------------------------------------------------------------------
  
 CEvents::CEvents(const CTxOut &vout)
 {
     COptCCParams p;
     // Get info from opt params
     if (IsPayToCryptoCondition(vout.scriptPubKey, p) && p.IsValid() && p.evalCode == EVAL_CUSTOM && p.version == 'E')
     {
         FromVector(p.vData[0], *this);
     }
 }
 
 CGameCreate::CGameCreate(const CTxOut &vout)
 {
     COptCCParams p;
     // Get info from opt params
     if (IsPayToCryptoCondition(vout.scriptPubKey, p) && p.IsValid() && p.evalCode == EVAL_CUSTOM && p.version == 'C')
     {
         FromVector(p.vData[0], *this);
     }
 }

UniValue custom_rawtxresult(UniValue &result,std::string rawtx,int32_t broadcastflag)
{
    CTransaction tx;
    if ( rawtx.size() > 0 )
    {
        result.push_back(Pair("hex",rawtx));
        if ( DecodeHexTx(tx,rawtx) != 0 )
        {
            if ( broadcastflag != 0 && myAddtomempool(tx) != 0 )
                RelayTransaction(tx);
            result.push_back(Pair("txid",tx.GetHash().ToString()));
            result.push_back(Pair("result","success"));
        } else result.push_back(Pair("error","decode hex"));
    } else result.push_back(Pair("error","couldnt finalize CCtx"));
    return(result);
}

UniValue custom_func2(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    /* Create game tx. 
    normal vins 
    vout1 -> CC_CUSTOM main CC address: unspendable in validation. 
    version/funcid = 'F'
    // Returns a TXID that will be used to make this game address.
    */
    CPubKey playerpk; CAmount totalin;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight()); std::string rawtx;
    playerpk = pubkey2pk(Mypubkey());
    std::string name = "atestname";
    UniValue result(UniValue::VOBJ); int64_t amount = CUSTOM_TXFEE; int32_t broadcastflag=0;
    if ( txfee == 0 )
        txfee = CUSTOM_TXFEE;
    if ( AddNormalinputs(mtx,playerpk,txfee*2,64) >= txfee*2 ) // add utxo to mtx
    {
        CGameCreate GameObj = CGameCreate(name, playerpk);
        mtx.vout.push_back(MakeCC1of1Vout('C',cp->evalcode,amount,GetUnspendable(cp,0),GameObj));
        rawtx = FinalizeCCTx(0,cp,mtx,playerpk,txfee,CScript());
        return(custom_rawtxresult(result,rawtx,broadcastflag));
    }
    return(result);
}

UniValue custom_func0(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    /* Fund Diablo address for playerpk 
    normal imputs 
    X outputs depnding on amount sent. 
    send in amounts equal to tx fee 
    must be to playerpk CC_CUSTOM address 
    version/funcid = 'F'
    */
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    UniValue result(UniValue::VOBJ);
    int number = 10; // number of utxos to fund. 
    CPubKey gametxidpk, playerpk; char gametxidaddr[64]={0};
    playerpk = pubkey2pk(Mypubkey());
    if ( txfee == 0 )
        txfee = CUSTOM_TXFEE;
    if ( AddNormalinputs(mtx,playerpk,txfee*2*number,64) >= txfee*2*number )
    {
        for ( int i = 0; i < number; i++ )
        {
            CGameCreate dummy = CGameCreate();
            mtx.vout.push_back(MakeCC1of1Vout('F', cp->evalcode, txfee*2, playerpk, dummy));
        }
        return(custom_rawtxresult(result,FinalizeCCTx(0,cp,mtx,playerpk,txfee,CScript()),1));
    }
    return 0;
}

UniValue custom_func1(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    /* Events, spend from  Diablo address for playerpk to the 
    gametxidpk + playerpk 1of2 address 
    validation must check that vins came from Diablo address for playerpk.
    tx will build up in this address and only playerpk can spend to it, anything else sent here is filtered out 
    This allows extraction of events objects direct from the addressindex, sorted by the counter. 
    Unless the player sends a spoofed tx with incorrect data it will only contain the correct data. 
    Players can only break their own games, no other person can do this without their private key. 
    */
    CPubKey gametxidpk, playerpk; char playeraddr[64]={0}; UniValue result(UniValue::VOBJ); int32_t broadcastflag=0;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight()); std::string rawtx;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs; CScript ccopret;
    
    uint256 gametxid = Parseuint256((char *)"014752e10b1634a62bd9b1caa39d8ded95ed1eab183768cdae2ee09bd913970a");
    gametxidpk = CCtxidaddr(playeraddr,gametxid);
    playerpk = pubkey2pk(Mypubkey());
    
    if ( txfee == 0 )
        txfee = CUSTOM_TXFEE;
    GetCCaddress(cp,playeraddr,playerpk);           // get address for EVAL_CUSTOM playerpk address
    SetCCunspents(unspentOutputs,playeraddr,true);  // Get all unspent cc_vouts in this address. 
    // loop our vouts and add one to our tx. 
    for ( auto utxo : unspentOutputs )
    {
        // no need to check where funding came from, we are the only privkey that can spend these utxos 
        //fprintf(stderr, "txid.%s vout.%li scriptpk.%s sats.%li blockht.%i\n", utxo.first.txhash.GetHex().c_str(),utxo.first.index, utxo.second.script.ToString().c_str(), utxo.second.satoshis, utxo.second.blockHeight);
        CScript dummy; COptCCParams ccp;
        if ( utxo.second.satoshis == txfee*2 && utxo.second.script.IsPayToCryptoCondition(&dummy, ccp) != 0 && ccp.vKeys.size() == 1 )
        {
            fprintf(stderr, "txid.%s vout.%li ccp.vKeys.size.%li m.%i n.%i\n", utxo.first.txhash.GetHex().c_str(),utxo.first.index, ccp.vKeys.size(), ccp.m, ccp.n);
            mtx.vin.push_back(CTxIn(utxo.first.txhash,utxo.first.index,CScript()));
            break; // we only need 1 utxo for each event tx. 
        }
    }
    
    static int counter = 0; // just for now, game will supply increments 
    
    if ( mtx.vin.size() == 1 ) 
    {
        counter++;
        std::vector<uint8_t> events;
        events.push_back(1); // dummy event
        CEvents cEvent = CEvents(counter, gametxid, events);
        mtx.vout.push_back(MakeCC1of2Vout('E',EVAL_CUSTOM, txfee, playerpk, gametxidpk, cEvent));
        
        bool signSucess = SignCCtx(mtx);
        return(custom_rawtxresult(result,EncodeHexTx(mtx),0));
    }
    else 
    {
        // we failed to get any inputs make funding tx. 
    }
    return(result);
}

/*UniValue custom_func0(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    CPubKey gametxidpk, playerpk, pk; char gametxidaddr[64]={0}, coinaddr[64]={0}; uint256 gametxid;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs; 
    uint32_t counter;
    gametxid = Parseuint256((char *)"a7e04fce4d71b6447281e2659dbed657d5c922e0cec02bdcb8115574f26509d5");
    
    playerpk = pubkey2pk(Mypubkey());
    gametxidpk = CCtxidaddr(gametxidaddr,gametxid);
    GetCCaddress(cp,gametxidaddr,gametxidpk);
    
    SetCCunspents(unspentOutputs, gametxidaddr, true);
    fprintf(stderr, "unspendable address.%s \n",gametxidaddr);
    std::map <int32_t,std::pair<uint256,size_t>> mUtxos;
    for ( auto utxo : unspentOutputs )
    {
        //fprintf(stderr, "txid.%s vout.%li scriptpk.%s sats.%li blockht.%i\n", utxo.first.txhash.GetHex().c_str(),utxo.first.index, utxo.second.script.ToString().c_str(), utxo.second.satoshis, utxo.second.blockHeight);
        CScript ccopret;
        if ( getCCopret(utxo.second.script,ccopret) && custom_opretdecode(ccopret,pk,counter) == '1' )
        {
            fprintf(stderr, "counter.%i\n", counter);
            mUtxos[counter] = std::make_pair(utxo.first.txhash,utxo.first.index);
        }
    }
    for ( auto utxo : mUtxos )
    {
        fprintf(stderr, "counter.%i txid.%s vout.%li\n", utxo.first, utxo.second.first.GetHex().c_str(), utxo.second.second);
    }
    return 0;
    
    // does a spendable address! 
    // get this working and figure out how to sign the tx with SignStepCC
    //GetCCaddress1of2(cp,coinaddr,playerpk,gametxidpk);
}

// send yourself 1 coin to your CC address using normal utxo from your -pubkey

UniValue custom_func1(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    CPubKey gametxidpk, playerpk; char gametxidaddr[64]={0}, coinaddr[64]={0}; uint256 gametxid; CAmount totalin;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight()); std::string rawtx;
    gametxid = Parseuint256((char *)"a7e04fce4d71b6447281e2659dbed657d5c922e0cec02bdcb8115574f26509d5");
    playerpk = pubkey2pk(Mypubkey());
    gametxidpk = CCtxidaddr(gametxidaddr,gametxid);
    GetCCaddress(cp,gametxidaddr,gametxidpk);
    UniValue result(UniValue::VOBJ); CPubKey mypk; int64_t amount = CUSTOM_TXFEE; int32_t broadcastflag=1;
    static int counter = 0;
    if ( txfee == 0 )
        txfee = CUSTOM_TXFEE;
    mypk = pubkey2pk(Mypubkey());
    if ( (totalin= AddNormalinputs(mtx,mypk,txfee*2,64)) >= txfee*2 ) // add utxo to mtx
    {
        // make op_return payload as normal. 
        OLD SHIT lol
        CScript opret = custom_opret('1',mypk,counter);
        std::vector<std::vector<unsigned char>> vData = std::vector<std::vector<unsigned char>>();
        if ( makeCCopret(opret, vData) )
        {
            // make vout0 with op_return included as payload.
            mtx.vout.push_back(MakeCC1vout(cp->evalcode,amount,gametxidpk,&vData));
            //fprintf(stderr, "vout size2.%li\n", mtx.vout.size());
            rawtx = FinalizeCCTx(0,cp,mtx,mypk,txfee,CScript());
            counter++;
            return(custom_rawtxresult(result,rawtx,broadcastflag));
        }
        fprintf(stderr, "counter.%i\n", counter);
        counter++;
        std::vector<uint8_t> events;
        events.push_back(1);
        CEvents cEvent = CEvents(counter, gametxid, events);
        mtx.vout.push_back(MakeCC1of2Vout(EVAL_CUSTOM, amount, mypk, gametxidpk, cEvent);
        CAmount change = totalin - txfee*2;
        mtx.vout.push_back(CTxOut(change, CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG));
        
    }
    return(result);
} */

bool custom_validate(struct CCcontract_info *cp,int32_t height,Eval *eval,const CTransaction tx)
{
    return true;
}
