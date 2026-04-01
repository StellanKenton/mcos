/***********************************************************************************
* @file     : framepareser_por.c
* @brief    : Project port helpers for the stream packet parser.
* @details  : Provides the default millisecond tick source used by timeout logic.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
**********************************************************************************/
#include "framepareser_por.h"

#include "rep_config.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

typedef struct stFrmPsrPorSlot {
    stFrmPsrFmt fmt;
    bool isUsed;
} stFrmPsrPorSlot;

static stFrmPsrPorSlot gFrmPsrPorSlots[FRM_PSR_POR_MAX_FMTS];

uint32_t frmPsrPorGetTickMs(void)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
    return 0U;
#endif
}

void frmPsrPorApplyDftCfg(stFrmPsrCfg *cfg)
{
    if (cfg == NULL) {
        return;
    }

    if (cfg->minHeadLen == 0U) {
        cfg->minHeadLen = cfg->headPatLen;
    }

    if (cfg->waitPktToutMs == 0U) {
        cfg->waitPktToutMs = FRM_PSR_POR_WAIT_PKT_TOUT_MS;
    }

    if (cfg->getTick == NULL) {
        cfg->getTick = frmPsrPorGetTickMs;
    }
}

void frmPsrPorApplyDftRunCfg(stFrmPsrRunCfg *runCfg)
{
    if (runCfg == NULL) {
        return;
    }

    if (runCfg->waitPktToutMs == 0U) {
        runCfg->waitPktToutMs = FRM_PSR_POR_WAIT_PKT_TOUT_MS;
    }

    if (runCfg->getTick == NULL) {
        runCfg->getTick = frmPsrPorGetTickMs;
    }
}

uint32_t frmPsrPorGetFmtCnt(void)
{
    return FRM_PSR_POR_MAX_FMTS;
}

bool frmPsrPorSetFmt(uint32_t idx, const stFrmPsrFmt *fmt)
{
    if ((idx >= FRM_PSR_POR_MAX_FMTS) || (!frmPsrIsFmtValid(fmt))) {
        return false;
    }

    gFrmPsrPorSlots[idx].fmt = *fmt;
    gFrmPsrPorSlots[idx].isUsed = true;
    return true;
}

const stFrmPsrFmt *frmPsrPorGetFmt(uint32_t idx)
{
    if ((idx >= FRM_PSR_POR_MAX_FMTS) || (!gFrmPsrPorSlots[idx].isUsed)) {
        return NULL;
    }

    return &gFrmPsrPorSlots[idx].fmt;
}

eFrmPsrSta frmPsrPorInit(stFrmPsr *psr, stRingBuffer *ringBuf, stFrmPsrCfg *cfg)
{
    if (cfg == NULL) {
        return FRM_PSR_INVALID_ARG;
    }

    frmPsrPorApplyDftCfg(cfg);
    return frmPsrInit(psr, ringBuf, cfg);
}

eFrmPsrSta frmPsrPorInitFmt(stFrmPsr *psr, stRingBuffer *ringBuf, uint32_t idx, stFrmPsrRunCfg *runCfg)
{
    const stFrmPsrFmt *lFmt;

    if (runCfg == NULL) {
        return FRM_PSR_INVALID_ARG;
    }

    lFmt = frmPsrPorGetFmt(idx);
    if (lFmt == NULL) {
        return FRM_PSR_FMT_INVALID;
    }

    frmPsrPorApplyDftRunCfg(runCfg);
    return frmPsrInitFmt(psr, ringBuf, lFmt, runCfg);
}

eFrmPsrSta frmPsrPorSelFmt(stFrmPsr *psr, uint32_t idx)
{
    const stFrmPsrFmt *lFmt;

    lFmt = frmPsrPorGetFmt(idx);
    if (lFmt == NULL) {
        return FRM_PSR_FMT_INVALID;
    }

    return frmPsrSelFmt(psr, lFmt);
}

eFrmPsrSta frmPsrPorMkPkt(uint32_t idx, const uint8_t *payloadBuf, uint16_t payloadLen, uint8_t *pktBuf, uint16_t pktBufSize, uint16_t *pktLen)
{
    const stFrmPsrFmt *lFmt;

    lFmt = frmPsrPorGetFmt(idx);
    if (lFmt == NULL) {
        return FRM_PSR_FMT_INVALID;
    }

    return frmPsrMkPktByFmt(lFmt, payloadBuf, payloadLen, pktBuf, pktBufSize, pktLen);
}
/**************************End of file********************************/

