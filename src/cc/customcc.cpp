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

int64_t custom_GetBets(struct CCcontract_info *cp, const uint256 &createtxid, const CCreate &GameObj, std::map <CPubKey, CAmount> &mPubKeyAmounts, int64_t &secondsleft, int32_t &totalPubKeys)
{
    char gameaddr[64]; CPubKey txidpk, ccpubkey; std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs; CAmount total = 0; CPubKey pubkey;
    txidpk = CCtxidaddr(gameaddr,createtxid);
    ccpubkey = GetUnspendable(cp,0);
    GetCCaddress1of2(cp,gameaddr,txidpk,ccpubkey);
    SetCCunspents(unspentOutputs,gameaddr,true);
    secondsleft = GameObj.timestamp-komodo_heightstamp(chainActive.Height());
    for ( auto utxo : unspentOutputs )
    {
        //fprintf(stderr, "txid.%s vout.%li scriptpk.%s sats.%li blockht.%i\n", utxo.first.txhash.GetHex().c_str(),utxo.first.index, utxo.second.script.ToString().c_str(), utxo.second.satoshis, utxo.second.blockHeight);
        CBet BetObj;
        if ( IsValidObject(utxo.second.script, BetObj) && BetObj.satoshis == utxo.second.satoshis )
        {
            //fprintf(stderr, "createtxid.%s sats.%li pubkey.%s funcid.%i\n", BetObj.createtxid.GetHex().c_str(), BetObj.satoshis, HexStr(BetObj.payoutpubkey).c_str(), BetObj.funcid);
            pubkey = BetObj.payoutpubkey;
            std::map <CPubKey, CAmount>::iterator pos = mPubKeyAmounts.find(pubkey);
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
    return total;
}

bool custom_hasResult(struct CCcontract_info *cp, const CCreate &GameObj, const uint256 &createtxid, CPubKey &winner, CAmount &totalAmountBet)
{
    std::string symbol; Notarisation nota1, nota2; std::map <CPubKey, CAmount> mPubKeyAmounts; int64_t secondsleft; int32_t totalPubKeys; CAmount total;
    symbol.assign(ASSETCHAINS_SYMBOL);
    int32_t n, i; int64_t x, y;
    // find the first block past the timestamp, then scan forwards to get a notarized notarization.
    for ( i = chainActive.Height(); i > 0; i-- )
    {
        if ( chainActive[i]->nTime < GameObj.timestamp )
            break;
    } /*
    if ( i == 0 )
        return false;
    i++; // block after the one found is first past the timestamp.
    if ( (n= ScanNotarisationsDBForwards(i, symbol, 1440, nota1)) == 0 )
        return false;
    // found first notarizaton after the timestamp, keep scanning forwards for the next one.
    if ( ScanNotarisationsDBForwards(n+1, symbol, 1440, nota2) == 0 )
        return false;

    // Create the random number from this notarization.
    memcpy(&x,&nota1.second.MoMoM ,sizeof(x)); */

    uint256 blockHash = *chainActive[i-20]->phashBlock;
    memcpy(&x,&blockHash ,sizeof(x));
    if ( x < 0 ) x = -x;

    // Sum all the pubkeys balances same as in status RPC.
    if ( (total= custom_GetBets(cp, createtxid, GameObj, mPubKeyAmounts, secondsleft, totalPubKeys)) == 0 )
        return false;

    // Cacluate the odds from the amounts bet
    int64_t bestChance = std::numeric_limits<int64_t>::max();
    for ( auto element : mPubKeyAmounts )
    {
        memcpy(&y,&element.first,sizeof(y));
        if ( y < 0 ) y = -y;
        //fprintf(stderr, "y.%li vs x.%li\n",y,x);
        int64_t chance = x - y;
        if ( chance < 0 ) chance = -chance;
        //fprintf(stderr, "chance.%li vs rndnumber.%li amountbet.%li\n",chance, x, element.second);
        int64_t weight = (element.second * 100) / total;
        int64_t adjustedchance = chance / weight;
        fprintf(stderr, "weight.%li adjusted chance.%li\n", weight, adjustedchance);
        if ( adjustedchance < bestChance )
        {
             bestChance = adjustedchance;
             winner = element.first;
             totalAmountBet = element.second;
        }
        fprintf(stderr, "bestchoice.%li pubkey.%s\n", bestChance, HexStr(winner).c_str());
    }
    return bestChance != std::numeric_limits<int64_t>::max();
}

UniValue custom_status(uint64_t txfee,struct CCcontract_info *cp,cJSON *params)
{
    UniValue result(UniValue::VOBJ); UniValue pubkeys(UniValue::VARR); uint256 createtxid = zeroid, hashBlock; CTransaction createtx; CPubKey winner;
    CAmount total, totalAmountBet; int64_t secondsleft;
    std::map <CPubKey, CAmount> mPubKeyAmounts; int32_t totalPubKeys = 0;
    if ( params != 0 && cJSON_GetArraySize(params) == 1 )
    {
        createtxid = juint256(jitem(params,0));
        if ( createtxid != zeroid && myGetTransaction(createtxid, createtx, hashBlock) != 0 )
        {
            CCreate GameObj; COptCCParams ccp;
            if ( createtx.vout.size() > 0 && IsPayToCryptoCondition(createtx.vout[0].scriptPubKey, ccp, GameObj) && GameObj.IsValid() )
            {
                if ( (total= custom_GetBets(cp, createtxid, GameObj, mPubKeyAmounts, secondsleft, totalPubKeys)) != 0 )
                {
                    for ( auto element : mPubKeyAmounts )
                    {
                        UniValue pubkeyobj(UniValue::VOBJ);
                        pubkeyobj.push_back(Pair(HexStr(element.first), element.second));
                        pubkeys.push_back(pubkeyobj);
                    }
                    result.push_back(Pair("pubkeys",pubkeys));
                    result.push_back(Pair("total_funds", total));
                    result.push_back(Pair("total_pubkeys", totalPubKeys));
                    result.push_back(Pair("end_time", GameObj.timestamp));
                    if ( secondsleft > 0 )
                        result.push_back(Pair("seconds_left", secondsleft));
                }
                if ( secondsleft < 0 )
                {
                    if ( custom_hasResult(cp, GameObj, createtxid, winner, totalAmountBet) )
                    {
                        result.push_back(Pair("winner", HexStr(winner)));
                        result.push_back(Pair("total_amount_bet", totalAmountBet));
                        result.push_back(Pair("claimed", total == 0 ? "true" : "false"));
                    }
                    else
                    {
                        result.push_back("Waiting for next notarized MoMoM hash...");
                    }
                }
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
    /* Payout transaction
    must determine if a result is possible using custom_hasResult function
    then construct a transaction paying the winner

    max amount of ccvins possible all vins must be valid bet objects for this game
    1 vout to the winning pubkey
    no other vouts allowed.

    */
    UniValue result(UniValue::VOBJ); uint256 createtxid = zeroid, hashBlock; CTransaction createtx; CPubKey txidpk, winner, ccpubkey; char gameaddr[64]; CAmount totalAmountBet, totalWithdrawn=0;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight()); std::string rawtx; int32_t broadcastflag=0, numberVins=0, maxVins;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs; CWithdraw withdrawObj;
    if ( txfee == 0 )
        txfee = CUSTOM_TXFEE;

    if ( params != 0 && cJSON_GetArraySize(params) == 1 )
    {
        createtxid = juint256(jitem(params,0));
        // Allow temporary specification of amount of vins to use, so we can find the limit of vins.
        maxVins = jint(jitem(params,1),0);
        if ( createtxid != zeroid && myGetTransaction(createtxid, createtx, hashBlock) != 0 )
        {
            CCreate GameObj; COptCCParams ccp;
            if ( createtx.vout.size() > 0 && IsPayToCryptoCondition(createtx.vout[0].scriptPubKey, ccp, GameObj) && GameObj.IsValid() )
            {
                if ( GameObj.timestamp < komodo_heightstamp(chainActive.Height()) )
                {
                    // add inputs until we reach the maximum possible amount of inputs.
                    txidpk = CCtxidaddr(gameaddr,createtxid);
                    ccpubkey = GetUnspendable(cp,0);
                    GetCCaddress1of2(cp,gameaddr,txidpk,ccpubkey);
                    SetCCunspents(unspentOutputs,gameaddr,true);
                    for ( auto utxo : unspentOutputs )
                    {
                        //fprintf(stderr, "txid.%s vout.%li scriptpk.%s sats.%li blockht.%i\n", utxo.first.txhash.GetHex().c_str(),utxo.first.index, utxo.second.script.ToString().c_str(), utxo.second.satoshis, utxo.second.blockHeight);
                        CBet BetObj; CWithdraw tempWithdrawObj;
                        // check for a valid withdraw object, if one exists we need to send it back to game address.
                        if ( IsValidObject(utxo.second.script, tempWithdrawObj) && tempWithdrawObj.createtxid == createtxid )
                        {
                            // if withdraw exists, we need to validate its not a fake vout.
                            // to do this, we need to fetch the tx from disk that sent it, and make sure it is in a tx that contains only valid bet object vins.
                            // this means it has passed validation and the winner is known.
                            // once we know this, we can simply pass this object to a new vout in this transaction
                            CTransaction withdrawtx;
                            if ( myGetTransaction(utxo.first.txhash, withdrawtx, hashBlock) != 0 )
                            {
                                for ( auto vin : withdrawtx.vin )
                                {
                                    CTransaction vintx; CBet vinBetObj;
                                    if ( myGetTransaction(vin.prevout.hash, vintx, hashBlock) != 0 )
                                    {
                                        if ( !IsValidObject(vintx.vout[vin.prevout.n].scriptPubKey, vinBetObj) || vinBetObj.createtxid != createtxid )
                                            return(cclib_error(result,"invalid withdraw transaction"));
                                    } else return(cclib_error(result,"could not fetch vintx"));
                                }
                                withdrawObj = tempWithdrawObj;
                            }
                        }
                        if ( IsValidObject(utxo.second.script, BetObj) && BetObj.satoshis == utxo.second.satoshis && BetObj.createtxid == createtxid )
                        {
                            // we know this utxo is in the right address and it contains the correct size satoshies value so add it as vin.
                            // stop adding vins once max is reached. We should test this and find the maximum number before the tx fails to send, validation will handle any number.
                            // Make sure the vout wont be oversized, this may need to be less than this not sure, have to test also.
                            if ( numberVins < maxVins && KOMODO_VALUETOOBIG(totalWithdrawn+utxo.second.satoshis) == 0 )
                            {
                                mtx.vin.push_back(CTxIn(utxo.first.txhash, utxo.first.index));
                                numberVins++;
                                totalWithdrawn += utxo.second.satoshis;
                            }
                        }
                    }
                    if ( mtx.vin.size() > 0 )
                    {
                        if ( withdrawObj.IsValid() )
                        {
                            // we have a valid withdraw object from a previous withdraw so send it back to the address.
                            mtx.vout.push_back(MakeCC1of2Vout('W', EVAL_CUSTOM, txfee, txidpk, ccpubkey, withdrawObj));
                            winner = withdrawObj.winner;
                        }
                        else
                        {
                            // There has not yet been a withdraw so get the result, and create the withdraw object
                            if ( custom_hasResult(cp, GameObj, createtxid, winner, totalAmountBet) )
                            {
                                withdrawObj.createtxid = createtxid;
                                withdrawObj.funcid = 'W';
                                withdrawObj.winner = winner;
                                mtx.vout.push_back(MakeCC1of2Vout('W', EVAL_CUSTOM, txfee, txidpk, ccpubkey, withdrawObj));
                            }
                            else return(cclib_error(result,"there is no result yet"));
                        }
                        // add the payout vout
                        mtx.vout.push_back(CTxOut(totalWithdrawn-txfee*2,CScript() << ParseHex(HexStr(winner)) << OP_CHECKSIG));
                        // sign the tx and send it.
                        if ( SignCCtx(mtx) )
                            return(custom_rawtxresult(result,EncodeHexTx(mtx),broadcastflag));
                        else return(cclib_error(result,"signing error"));
                    } else return(cclib_error(result,"no vins found"));
                } else return(cclib_error(result,"this game has no result yet"));
            } else return(cclib_error(result,"createtxid is not valid"));
        } else return(cclib_error(result,"createtxid not found"));
    } else return(cclib_error(result,"not enough parameters"));
    return result;
}

bool custom_validate(struct CCcontract_info *cp,int32_t height,Eval *eval,const CTransaction tx)
{
    /* Validation
    first loop vins check if only 1 create OR all bets
    if create, return false, cannot be spent
    if 1 bet, all others must be a bet

    Bet Object must only be spent if a result is known.
        If a bet object vin is detected, all of the vins must also be valid bets from same game address.
        There must only be one vout, that pays the winning pubkey.
        if all this is met call the hasResult function.
        then make sure the only vout is paying the winning pubkey.

    changes to use withdraw object
        first withdraw uses the hasResult function, compares the withdraw object contains the correct winner.
        hash the withdraw object from vin, and the withdraw object from vout, to verify withdraw obj is correct.
    */
    uint256 createtxid = zeroid, withdrawHash = zeroid; CScript gameScriptPub, testScriptPub;
    CCreate GameObj; CBet BetObj; CWithdraw withdrawObj; CPubKey winner; int64_t totalAmountBet;

    // Load the coins view to get the previous vouts fast!
    CCoinsView dummy;
    CCoinsViewCache view(&dummy);
    CCoinsViewMemPool viewMemPool(pcoinsTip, mempool);
    view.SetBackend(viewMemPool);

    if ( tx.vout.size() != 2 )
        return(eval->Invalid("wrong number of vouts"));

    if ( !IsValidObject(tx.vout[0].scriptPubKey, withdrawObj) )
        return(eval->Invalid("invalid withdraw object"));

    // Check vins so we know what type of transaction this is, and verify that all vins are spending from the same game.
    for ( auto vin : tx.vin )
    {
        const CTxOut &prevout = view.GetOutputFor(vin);
        if ( IsValidObject(prevout.scriptPubKey, GameObj) )
        {
            return(eval->Invalid("cannot spend create vout"));
        }
        else if ( IsValidObject(prevout.scriptPubKey, BetObj) && prevout.scriptPubKey.IsPayToCryptoCondition(&gameScriptPub) )
        {
            if ( createtxid == zeroid )
            {
                // first bet object seen, now all others must contain the same createtxid and reside in the same address.
                createtxid = BetObj.createtxid;
                testScriptPub = gameScriptPub;
            }
            else if ( BetObj.createtxid != createtxid )
                return(eval->Invalid("wrong createtxid"));
            else if ( gameScriptPub != testScriptPub )
                return(eval->Invalid("vin in wrong address"));
        }
        else if ( IsValidObject(prevout.scriptPubKey, withdrawObj) && prevout.scriptPubKey.IsPayToCryptoCondition(&gameScriptPub) )
        {
            if ( createtxid == zeroid )
            {
                createtxid = withdrawObj.createtxid;
                testScriptPub = gameScriptPub;
            }
            if ( withdrawObj.createtxid != createtxid )
                return(eval->Invalid("wrong createtxid"));
            // if we have a withdraw object hash it, to compare to the vout withdrawObj later.
            if ( withdrawHash != zeroid )
                return(eval->Invalid("cannot have more than 1 withdraw vout"));
            withdrawHash = GetHash(withdrawObj);
        }
        else
        {
            return(eval->Invalid("invalid vin"));
        }
    }

    // Get the Game Object
    CCoins createCoins;
    view.GetCoins(createtxid, createCoins);
    if ( createCoins.vout.size() > 0 && IsValidObject(createCoins.vout[0].scriptPubKey, GameObj) )
    {
        // If there is a result get the wining pubkey.
        if ( GameObj.timestamp < komodo_heightstamp(height) )
            return(eval->Invalid("chain not past game timestamp"));
        if ( withdrawHash == zeroid )
        {
            // There is no withdraw object vin, so we need to validate the withdraw object in vouts is valid.
            if ( !custom_hasResult(cp, GameObj, createtxid, winner, totalAmountBet) )
                return(eval->Invalid("game has no result yet"));
            else if ( withdrawObj.createtxid != createtxid )
                return(eval->Invalid("wrong createtxid"));
            else if ( withdrawObj.winner != winner )
                return(eval->Invalid("wrong winner"));
        }
        else
        {
            // we have a spent withdraw object need to validate that it matches the withdraw in vouts.
            if ( GetHash(withdrawObj) != withdrawHash )
                return(eval->Invalid("withdrawObj hash is wrong"));
            winner = withdrawObj.winner;
        }
    } else return(eval->Invalid("invalid game"));

    // We got this far so a winner needs to be paid. Check that it was!
    CPubKey testWinner (tx.vout[1].scriptPubKey.begin()+1, tx.vout[1].scriptPubKey.end()-1);
    if ( winner == testWinner )
        return true;
     else return(eval->Invalid("pays wrong pubkey"));
}
