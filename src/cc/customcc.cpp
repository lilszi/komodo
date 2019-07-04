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
  
CCreate::CCreate(const CScript &scriptPubKey)
{
    COptCCParams p;
    // Get info from opt params
    // version acts as funcid
    if (IsPayToCryptoCondition(scriptPubKey, p) && p.IsValid() && p.evalCode == EVAL_CUSTOM && p.version == 'C')
    {
        FromVector(p.vData[0], *this);
        if ( this->funcid != 'C' )
        {
            // funcid mismatch set name to 0, to invalidate this object. 
            name = "";
        }
    }
}
 
CBet::CBet(const CScript &scriptPubKey)
{
    COptCCParams p;
    // Get info from opt params
    if (IsPayToCryptoCondition(scriptPubKey, p) && p.IsValid() && p.evalCode == EVAL_CUSTOM && p.version == 'B')
    {
        FromVector(p.vData[0], *this);
        if ( this->funcid != 'B' )
        {
            // funcid mismatch invalidate this object. 
            createtxid = zeroid;
        }
    }
}
 
CWithdraw::CWithdraw(const CScript &scriptPubKey)
{
    COptCCParams p;
    // Get info from opt params
    if (IsPayToCryptoCondition(scriptPubKey, p) && p.IsValid() && p.evalCode == EVAL_CUSTOM && p.version == 'W')
    {
        FromVector(p.vData[0], *this);
        if ( this->funcid != 'W' )
        {
            // funcid mismatch invalidate this object. 
            createtxid = zeroid;
        }
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

UniValue custom_create(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    /* Create game tx. 
    normal vins 
    vout1 -> CC_CUSTOM main CC address: unspendable in validation. 
    version/funcid = 'C'
    Returns a TXID that will be used to make this game's address.
    */
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight()); std::string rawtx;
    CPubKey mypk = pubkey2pk(Mypubkey());
    std::string name; int32_t n; int64_t timestamp; CAmount total;
    UniValue result(UniValue::VOBJ); int32_t broadcastflag=1;
    if ( txfee == 0 )
        txfee = CUSTOM_TXFEE;
    
    if ( params != 0 && ((n= cJSON_GetArraySize(params)) == 2) )
    {
        name.assign(jstr(jitem(params,0),0));
        if ( name.empty() )
        {
            return(cclib_error(result,"name cannot be empty"));
        }
        timestamp = juint(jitem(params,1),0);
        if ( timestamp < komodo_heightstamp(chainActive.Height()+ASSETCHAINS_BLOCKTIME*20) )
        {
            fprintf(stderr, "now.%li vs timestamp.%li now+25blocks.%li\n", time(NULL), timestamp, (time(NULL)+ASSETCHAINS_BLOCKTIME*25));
            return(cclib_error(result,"finish time must be at least 20 blocks in the future."));
        }
        if ( (total= AddNormalinputs(mtx,mypk,txfee*2,64)) >= txfee*2 ) // add utxo to mtx
        {
            uint8_t funcid = 'C';
            CCreate GameObj = CCreate(funcid, name, timestamp);
            mtx.vout.push_back(MakeCC1of1Vout(funcid,cp->evalcode,txfee,GetUnspendable(cp,0),GameObj));
            CAmount change = total - 2*txfee;
            mtx.vout.push_back(CTxOut(change,CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG));
            if ( SignCCtx(mtx) )
                return(custom_rawtxresult(result,EncodeHexTx(mtx),broadcastflag));
            else return(cclib_error(result,"signing error"));
        }
    } else return(cclib_error(result,"not enough parameters"));
    return(result);
}

UniValue custom_bet(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    /* Bet transaction
    normal vins 
    vout1 -> createtxid address, 1of2 with global pubkey to allow winner to spend these inputs. 
    version/funcid = 'B'
    json args is just createtxid 
    */
    UniValue result(UniValue::VOBJ); uint256 createtxid = zeroid, hashBlock; CTransaction createtx; CPubKey txidpk; char gameaddr[64]; CAmount amount = 0, total;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight()); std::string rawtx; int32_t broadcastflag=1;
    CPubKey mypk = pubkey2pk(Mypubkey());
    
    if ( params != 0 && cJSON_GetArraySize(params) == 2 )
    {
        createtxid = juint256(jitem(params,0));
        amount = jdouble(jitem(params,1),0)*COIN + 0.0000000049;
        if ( amount < 0 ) 
            return(cclib_error(result,"amount cannot be zero"));
        if ( createtxid != zeroid && myGetTransaction(createtxid, createtx, hashBlock) != 0 )
        {
            CCreate GameObj; COptCCParams ccp;
            if ( createtx.vout.size() > 0 && IsPayToCryptoCondition(createtx.vout[0].scriptPubKey, ccp, GameObj) && GameObj.IsValid() )
            {
                if ( GameObj.timestamp > komodo_heightstamp(chainActive.Height()) )
                {
                    // we will need a refund path, for bets that are confirmed after the result is known
                    if ( (total= AddNormalinputs(mtx,mypk,amount+txfee,64)) >= amount+txfee ) 
                    {
                        uint8_t funcid = 'B';
                        txidpk = CCtxidaddr(gameaddr,createtxid);
                        CBet BetObj = CBet(funcid, createtxid, amount, mypk);
                        mtx.vout.push_back(MakeCC1of2Vout(funcid, EVAL_CUSTOM, amount, txidpk, GetUnspendable(cp,0), BetObj));
                        CAmount change = total - amount - txfee;
                        mtx.vout.push_back(CTxOut(change,CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG));
                        if ( SignCCtx(mtx) )
                            return(custom_rawtxresult(result,EncodeHexTx(mtx),broadcastflag));
                        else return(cclib_error(result,"signing error"));
                    } else return(cclib_error(result,"not enough normal inputs"));
                } else return(cclib_error(result,"this game is over"));
            } else return(cclib_error(result,"createtxid is not valid"));
        } else return(cclib_error(result,"createtxid not found"));
    } else return(cclib_error(result,"not enough parameters"));
    return result;
}

UniValue custom_status(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    UniValue result(UniValue::VOBJ); UniValue pubkeys(UniValue::VARR); uint256 createtxid = zeroid, hashBlock; CTransaction createtx; CPubKey txidpk, ccpubkey; char gameaddr[64];
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs; CAmount total = 0; std::string pubkey; int64_t timeleft;
    std::map <std::string, CAmount> mPubKeyAmounts; int32_t totalPubKeys = 0;
    if ( params != 0 && cJSON_GetArraySize(params) == 1 )
    {
        createtxid = juint256(jitem(params,0));
        if ( createtxid != zeroid && myGetTransaction(createtxid, createtx, hashBlock) != 0 )
        {
            CCreate GameObj; COptCCParams ccp;
            if ( createtx.vout.size() > 0 && IsPayToCryptoCondition(createtx.vout[0].scriptPubKey, ccp, GameObj) && GameObj.IsValid() )
            {
                txidpk = CCtxidaddr(gameaddr,createtxid);
                ccpubkey = GetUnspendable(cp,0);
                GetCCaddress1of2(cp,gameaddr,txidpk,ccpubkey);
                SetCCunspents(unspentOutputs,gameaddr,true);
                secondsleft =  GameObj.timestamp-komodo_heightstamp(chainActive.Height());
                for ( auto utxo : unspentOutputs )
                {
                    //fprintf(stderr, "txid.%s vout.%li scriptpk.%s sats.%li blockht.%i\n", utxo.first.txhash.GetHex().c_str(),utxo.first.index, utxo.second.script.ToString().c_str(), utxo.second.satoshis, utxo.second.blockHeight);
                    CBet BetObj; 
                    if ( IsValidObject(utxo.second.script, BetObj) )
                    {
                        //fprintf(stderr, "createtxid.%s sats.%li pubkey.%s funcid.%i\n", BetObj.createtxid.GetHex().c_str(), BetObj.satoshis, HexStr(BetObj.payoutpubkey).c_str(), BetObj.funcid);
                        pubkey = HexStr(BetObj.payoutpubkey);
                        std::map <std::string, CAmount>::iterator pos = mPubKeyAmounts.find(pubkey);
                        if ( pos == mPubKeyAmounts.end() )
                        {
                            // insert new address + utxo amount
                            mPubKeyAmounts[pubkey] = BetObj.satoshis;
                            totalPubKeys++;
                        }
                        else
                        {
                            // update unspent tally for this address
                            mPubKeyAmounts[pubkey] += BetObj.satoshis;
                        }
                        total += utxo.second.satoshis;
                    }
                }
                if ( total != 0 )
                {
                    for ( auto element : mPubKeyAmounts )
                    {
                        UniValue pubkeyobj(UniValue::VOBJ);
                        pubkeyobj.push_back(Pair(element.first, element.second));
                        pubkeys.push_back(pubkeyobj);
                    }
                    result.push_back(Pair("pubkeys",pubkeys));
                    result.push_back(Pair("total_funds", total));
                    result.push_back(Pair("total_pubkeys", totalPubKeys));
                    result.push_back(Pair("end_time", GameObj.timestamp));
                    if ( secondsleft > 0 )
                        result.push_back(Pair("seconds_left", secondsleft));
                    else 
                    {
                        // Here we should check if the game has been paid out or can be paid out. 
                        // If the game has a result, publish the winner. 
                        result.push_back("this game is finished");
                    }
                } 
                else if ( secondsleft < 0 )
                {
                    // It may have been paid out. Extract the winner and display the winning pubkey and the releasing pubkey.
                    
                }
                return(cclib_error(result,"no bets found"));
            } else return(cclib_error(result,"createtxid is not valid"));
        } else return(cclib_error(result,"createtxid not found"));
    } else return(cclib_error(result,"not enough parameters"));
    return result;
}

UniValue custom_list(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    bool fShowFinished = false; UniValue result(UniValue::VARR); char coinaddr[64]={0}; std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
    GetCCaddress(cp,coinaddr,GetUnspendable(cp,0));
    SetCCunspents(unspentOutputs,coinaddr,true);
    
    if ( params != 0 )
        fShowFinished = true;
    
    for ( auto utxo : unspentOutputs )
    {
        //fprintf(stderr, "txid.%s vout.%li scriptpk.%s sats.%li blockht.%i\n", utxo.first.txhash.GetHex().c_str(),utxo.first.index, utxo.second.script.ToString().c_str(), utxo.second.satoshis, utxo.second.blockHeight);
        CCreate GameObj;
        if ( IsValidObject( utxo.second.script, GameObj) )
        {
            //fprintf(stderr, "gameobj.name.%s gameobj.timestamp.%li funcid.%i\n",GameObj.name.c_str(), GameObj.timestamp, GameObj.funcid);
            if ( fShowFinished )
                result.push_back(utxo.first.txhash.GetHex());
            else if ( GameObj.timestamp > komodo_heightstamp(chainActive.Height()) )
                result.push_back(utxo.first.txhash.GetHex());
        }
    }
    return result;
}

UniValue custom_withdraw(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    UniValue result(UniValue::VOBJ); CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight()); std::string rawtx; int32_t broadcastflag=0;
    
    return result;
}

bool custom_validate(struct CCcontract_info *cp,int32_t height,Eval *eval,const CTransaction tx)
{
    return true;
}
