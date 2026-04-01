/***********************************************************************************
* @file     : framepareser_port.c
* @brief    : Project port helpers for the stream packet parser.
* @details  : Provides the default millisecond tick source used by timeout logic.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
**********************************************************************************/
#include "framepareser_port.h"

#include "rep_config.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

typedef struct stFrmPsrPortSlot {
    stFrmPsrFmt fmt;
    bool isUsed;
} stFrmPsrPortSlot;

static stFrmPsrPortSlot gFrmPsrPortSlots[FRM_PSR_PORT_MAX_FMTS];

uint32_t frmPsrPortGetTickMs(void)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
    return 0U;
#endif
}

void frmPsrPortApplyDftCfg(stFrmPsrCfg *cfg)
{
    if (cfg == NULL) {
        return;
    }

    if (cfg->minHeadLen == 0U) {
        cfg->minHeadLen = cfg->headPatLen;
    }

    if (cfg->waitPktToutMs == 0U) {
        cfg->waitPktToutMs = FRM_PSR_PORT_WAIT_PKT_TOUT_MS;
    }

    if (cfg->getTick == NULL) {
        cfg->getTick = frmPsrPortGetTickMs;
    }
}

void frmPsrPortApplyDftRunCfg(stFrmPsrRunCfg *runCfg)
{
    if (runCfg == NULL) {
        return;
    }

    if (runCfg->waitPktToutMs == 0U) {
        runCfg->waitPktToutMs = FRM_PSR_PORT_WAIT_PKT_TOUT_MS;
    }

    if (runCfg->getTick == NULL) {
        runCfg->getTick = frmPsrPortGetTickMs;
    }
}

uint32_t frmPsrPortGetFmtCnt(void)
{
    return FRM_PSR_PORT_MAX_FMTS;
}

bool frmPsrPortSetFmt(uint32_t idx, const stFrmPsrFmt *fmt)
{
    if ((idx >= FRM_PSR_PORT_MAX_FMTS) || (!frmPsrIsFmtValid(fmt))) {
        return false;
    }

    gFrmPsrPortSlots[idx].fmt = *fmt;
    gFrmPsrPortSlots[idx].isUsed = true;
    return true;
}

const stFrmPsrFmt *frmPsrPortGetFmt(uint32_t idx)
{
    if ((idx >= FRM_PSR_PORT_MAX_FMTS) || (!gFrmPsrPortSlots[idx].isUsed)) {
        return NULL;
    }

    return &gFrmPsrPortSlots[idx].fmt;
}

eFrmPsrSta frmPsrPortInit(stFrmPsr *psr, stRingBuffer *ringBuf, stFrmPsrCfg *cfg)
{
    if (cfg == NULL) {
        return FRM_PSR_INVALID_ARG;
    }

    frmPsrPortApplyDftCfg(cfg);
    return frmPsrInit(psr, ringBuf, cfg);
}

eFrmPsrSta frmPsrPortInitFmt(stFrmPsr *psr, stRingBuffer *ringBuf, uint32_t idx, stFrmPsrRunCfg *runCfg)
{
    const stFrmPsrFmt *lFmt;

    if (runCfg == NULL) {
        return FRM_PSR_INVALID_ARG;
    }

    lFmt = frmPsrPortGetFmt(idx);
    if (lFmt == NULL) {
        return FRM_PSR_FMT_INVALID;
    }

    frmPsrPortApplyDftRunCfg(runCfg);
    return frmPsrInitFmt(psr, ringBuf, lFmt, runCfg);
}

eFrmPsrSta frmPsrPortSelFmt(stFrmPsr *psr, uint32_t idx)
{
    const stFrmPsrFmt *lFmt;

    lFmt = frmPsrPortGetFmt(idx);
    if (lFmt == NULL) {
        return FRM_PSR_FMT_INVALID;
    }

    return frmPsrSelFmt(psr, lFmt);
}

eFrmPsrSta frmPsrPortMkPkt(uint32_t idx, const uint8_t *payloadBuf, uint16_t payloadLen, uint8_t *pktBuf, uint16_t pktBufSize, uint16_t *pktLen)
{
    const stFrmPsrFmt *lFmt;

    lFmt = frmPsrPortGetFmt(idx);
    if (lFmt == NULL) {
        return FRM_PSR_FMT_INVALID;
    }

    return frmPsrMkPktByFmt(lFmt, payloadBuf, payloadLen, pktBuf, pktBufSize, pktLen);
}
/**************************End of file********************************/
