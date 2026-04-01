/************************************************************************************
* @file     : framepareser_por.h
* @brief    : Project port helpers for the stream packet parser.
* @details  : Supplies default timing hooks and parser initialization helpers.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
***********************************************************************************/
#ifndef FRM_PSR_POR_H
#define FRM_PSR_POR_H

#include <stdbool.h>
#include <stdint.h>

#include "framepareser.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FRM_PSR_POR_WAIT_PKT_TOUT_MS
#define FRM_PSR_POR_WAIT_PKT_TOUT_MS    60U
#endif

#ifndef FRM_PSR_POR_MAX_FMTS
#define FRM_PSR_POR_MAX_FMTS            4U
#endif

uint32_t frmPsrPorGetTickMs(void);
void frmPsrPorApplyDftCfg(stFrmPsrCfg *cfg);
void frmPsrPorApplyDftRunCfg(stFrmPsrRunCfg *runCfg);
uint32_t frmPsrPorGetFmtCnt(void);
bool frmPsrPorSetFmt(uint32_t idx, const stFrmPsrFmt *fmt);
const stFrmPsrFmt *frmPsrPorGetFmt(uint32_t idx);
eFrmPsrSta frmPsrPorInit(stFrmPsr *psr, stRingBuffer *ringBuf, stFrmPsrCfg *cfg);
eFrmPsrSta frmPsrPorInitFmt(stFrmPsr *psr, stRingBuffer *ringBuf, uint32_t idx, stFrmPsrRunCfg *runCfg);
eFrmPsrSta frmPsrPorSelFmt(stFrmPsr *psr, uint32_t idx);
eFrmPsrSta frmPsrPorMkPkt(uint32_t idx, const uint8_t *payloadBuf, uint16_t payloadLen, uint8_t *pktBuf, uint16_t pktBufSize, uint16_t *pktLen);

#ifdef __cplusplus
}
#endif

#endif  // FRM_PSR_POR_H
/**************************End of file********************************/

