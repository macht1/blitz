#include "chain.h"
#include "dbwrapper.h"

bool IsInitialBlockDownload()
{
    LOCK(cs_main);
    if (pindexBest == NULL || nBestHeight < Checkpoints::GetTotalBlocksEstimate())
        return true;
    static int64_t nLastUpdate;
    static CBlockIndex* pindexLastBest;
    if (pindexBest != pindexLastBest)
    {
        pindexLastBest = pindexBest;
        nLastUpdate = GetTime();
    }
    return (GetTime() - nLastUpdate < 15 &&
            pindexBest->GetBlockTime() < GetTime() - 8 * 60 * 60);
}

CBlockIndex* FindBlockByHeight(int nHeight)
{
    CBlockIndex *pblockindex;
    if (nHeight < nBestHeight / 2)
        pblockindex = pindexGenesisBlock;
    else
        pblockindex = pindexBest;
    if (pblockindexFBBHLast && abs(nHeight - pblockindex->nHeight) > abs(nHeight - pblockindexFBBHLast->nHeight))
        pblockindex = pblockindexFBBHLast;
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;
    while (pblockindex->nHeight < nHeight)
        pblockindex = pblockindex->pnext;
    pblockindexFBBHLast = pblockindex;
    return pblockindex;
}


bool Reorganize(CDBWrapper& txdb, CBlockIndex* pindexNew)
{
    LogPrintf("REORGANIZE\n");

    // Find the fork
    CBlockIndex* pfork = pindexBest;
    CBlockIndex* plonger = pindexNew;
    while (pfork != plonger)
    {
        while (plonger->nHeight > pfork->nHeight)
            if (!(plonger = plonger->pprev))
                return error("Reorganize() : plonger->pprev is null");
        if (pfork == plonger)
            break;
        if (!(pfork = pfork->pprev))
            return error("Reorganize() : pfork->pprev is null");
    }

    // List of what to disconnect
    vector<CBlockIndex*> vDisconnect;
    for (CBlockIndex* pindex = pindexBest; pindex != pfork; pindex = pindex->pprev)
        vDisconnect.push_back(pindex);

    // List of what to connect
    vector<CBlockIndex*> vConnect;
    for (CBlockIndex* pindex = pindexNew; pindex != pfork; pindex = pindex->pprev)
        vConnect.push_back(pindex);
    reverse(vConnect.begin(), vConnect.end());

    LogPrintf("REORGANIZE: Disconnect %u blocks; %s..%s\n", vDisconnect.size(), pfork->GetBlockHash().ToString(), pindexBest->GetBlockHash().ToString());
    LogPrintf("REORGANIZE: Connect %u blocks; %s..%s\n", vConnect.size(), pfork->GetBlockHash().ToString(), pindexNew->GetBlockHash().ToString());

    // Disconnect shorter branch
    list<CTransaction> vResurrect;
    BOOST_FOREACH(CBlockIndex* pindex, vDisconnect)
    {
        CBlock block;
        if (!block.ReadFromDisk(pindex))
            return error("Reorganize() : ReadFromDisk for disconnect failed");
        if (!block.DisconnectBlock(txdb, pindex))
            return error("Reorganize() : DisconnectBlock %s failed", pindex->GetBlockHash().ToString());

        // Queue memory transactions to resurrect.
        // We only do this for blocks after the last checkpoint (reorganisation before that
        // point should only happen with -reindex/-loadblock, or a misbehaving peer.
        BOOST_REVERSE_FOREACH(const CTransaction& tx, block.vtx)
        if (!(tx.IsCoinBase() || tx.IsCoinStake()) && pindex->nHeight > Checkpoints::GetTotalBlocksEstimate())
            vResurrect.push_front(tx);
    }

    // Connect longer branch
    vector<CTransaction> vDelete;
    for (unsigned int i = 0; i < vConnect.size(); i++)
    {
        CBlockIndex* pindex = vConnect[i];
        CBlock block;
        if (!block.ReadFromDisk(pindex))
            return error("Reorganize() : ReadFromDisk for connect failed");
        if (!block.ConnectBlock(txdb, pindex))
        {
            // Invalid block
            return error("Reorganize() : ConnectBlock %s failed", pindex->GetBlockHash().ToString());
        }

        // Queue memory transactions to delete
        BOOST_FOREACH(const CTransaction& tx, block.vtx)
        vDelete.push_back(tx);
    }
    if (!txdb.WriteHashBestChain(pindexNew->GetBlockHash()))
        return error("Reorganize() : WriteHashBestChain failed");

    // Make sure it's successfully written to disk before changing memory structure
    if (!txdb.TxnCommit())
        return error("Reorganize() : TxnCommit failed");

    // Disconnect shorter branch
    BOOST_FOREACH(CBlockIndex* pindex, vDisconnect)
    if (pindex->pprev)
        pindex->pprev->pnext = NULL;

    // Connect longer branch
    BOOST_FOREACH(CBlockIndex* pindex, vConnect)
    if (pindex->pprev)
        pindex->pprev->pnext = pindex;

    // Resurrect memory transactions that were in the disconnected branch
    BOOST_FOREACH(CTransaction& tx, vResurrect)
    AcceptToMemoryPool(mempool, tx, false, NULL);

    // Delete redundant memory transactions that are in the connected branch
    BOOST_FOREACH(CTransaction& tx, vDelete) {
        mempool.remove(tx);
        mempool.removeConflicts(tx);
    }

    LogPrintf("REORGANIZE: done\n");

    return true;
}

void InvalidChainFound(CBlockIndex* pindexNew)
{
    if (pindexNew->nChainTrust > nBestInvalidTrust)
    {
        nBestInvalidTrust = pindexNew->nChainTrust;
        CDBWrapper().WriteBestInvalidTrust(CBigNum(nBestInvalidTrust));
    }

    uint256 nBestInvalidBlockTrust = pindexNew->nChainTrust - pindexNew->pprev->nChainTrust;
    uint256 nBestBlockTrust = pindexBest->nHeight != 0 ? (pindexBest->nChainTrust - pindexBest->pprev->nChainTrust) : pindexBest->nChainTrust;

    LogPrintf("InvalidChainFound: invalid block=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
              pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
              CBigNum(pindexNew->nChainTrust).ToString(), nBestInvalidBlockTrust.GetLow64(),
              DateTimeStrFormat("%x %H:%M:%S", pindexNew->GetBlockTime()));
    LogPrintf("InvalidChainFound:  current best=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
              hashBestChain.ToString(), nBestHeight,
              CBigNum(pindexBest->nChainTrust).ToString(),
              nBestBlockTrust.GetLow64(),
              DateTimeStrFormat("%x %H:%M:%S", pindexBest->GetBlockTime()));
}

